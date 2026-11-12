#include "knowledge_graph.hpp"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <tuple>

namespace
{
const cv::Scalar BACKGROUND(24, 21, 19);
const cv::Scalar TEXT(225, 225, 225);
const cv::Scalar MUTED(155, 155, 155);
const cv::Scalar GIVEN(70, 70, 235);
const cv::Scalar INFERRED(45, 155, 245);
const cv::Scalar EDGE(155, 170, 190);
const double FONT_SCALE = 0.5;
const double LABEL_SCALE = 0.45;
const int LINE_HEIGHT = 19;
const int PADDING = 12;
const int MIN_NODE_WIDTH = 400;
const int LAYER_GAP = 260;
const int ROW_GAP = 48;
const int TOP = 48;

std::vector<std::string> wrap_text(const std::string& text, int width, double scale = FONT_SCALE)
{
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string word;
    std::string line;
    auto fits = [=](const std::string& value)
    {
        return cv::getTextSize(value, cv::FONT_HERSHEY_SIMPLEX, scale, 1, nullptr).width <= width;
    };

    while (input >> word)
    {
        auto candidate = line.empty() ? word : line + " " + word;
        if (fits(candidate))
        {
            line = std::move(candidate);
            continue;
        }
        if (!line.empty())
        {
            lines.push_back(line);
            line.clear();
        }
        for (char character : word)
        {
            if (!line.empty() && !fits(line + character))
            {
                lines.push_back(line);
                line.clear();
            }
            line += character;
        }
    }
    if (!line.empty())
        lines.push_back(line);
    return lines;
}

int center_y(const KnowledgeGraphNode& node)
{
    return node.bounds.y + node.bounds.height / 2;
}

void place_layers(KnowledgeGraphLayout& graph, const std::vector<std::vector<size_t>>& layers)
{
    std::vector<int> heights(layers.size(), 0);
    for (size_t depth = 0; depth < layers.size(); depth++)
    {
        for (size_t node : layers[depth])
            heights[depth] += graph.nodes[node].bounds.height + ROW_GAP;
        if (!layers[depth].empty())
            heights[depth] -= ROW_GAP;
    }
    int height = *std::max_element(heights.begin(), heights.end());
    int left = 2 * PADDING;
    for (size_t depth = 0; depth < layers.size(); depth++)
    {
        int top = TOP + (height - heights[depth]) / 2;
        int width = MIN_NODE_WIDTH;
        for (size_t index : layers[depth])
        {
            auto& bounds = graph.nodes[index].bounds;
            bounds.x = left;
            bounds.y = top;
            width = std::max(width, bounds.width);
            top += bounds.height + ROW_GAP;
        }
        left += width + LAYER_GAP;
    }
}

void connect_edges(KnowledgeGraphLayout& graph)
{
    std::vector<std::vector<size_t>> outgoing(graph.nodes.size()), incoming(graph.nodes.size());
    for (size_t i = 0; i < graph.edges.size(); i++)
    {
        outgoing[graph.edges[i].from].push_back(i);
        incoming[graph.edges[i].to].push_back(i);
    }
    for (size_t node = 0; node < graph.nodes.size(); node++)
    {
        auto assign_ports = [&](std::vector<size_t>& edges, bool source)
        {
            std::stable_sort(edges.begin(), edges.end(), [&](size_t a, size_t b)
            {
                const auto& left = graph.edges[a];
                const auto& right = graph.edges[b];
                size_t left_other = source ? left.to : left.from;
                size_t right_other = source ? right.to : right.from;
                return std::make_tuple(center_y(graph.nodes[left_other]), left_other, left.fact_index)
                    < std::make_tuple(center_y(graph.nodes[right_other]), right_other, right.fact_index);
            });
            const auto& box = graph.nodes[node].bounds;
            for (size_t i = 0; i < edges.size(); i++)
            {
                cv::Point port(source ? box.br().x : box.x,
                    box.y + PADDING + (i + 1) * (box.height - 2 * PADDING) / (edges.size() + 1));
                if (source)
                    graph.edges[edges[i]].start = port;
                else
                    graph.edges[edges[i]].end = port;
            }
        };
        assign_ports(outgoing[node], true);
        assign_ports(incoming[node], false);
    }
}

void place_labels(KnowledgeGraphLayout& graph)
{
    for (auto& edge : graph.edges)
    {
        int width = 0;
        for (const auto& line : edge.label)
            width = std::max(width, cv::getTextSize(line, cv::FONT_HERSHEY_SIMPLEX, LABEL_SCALE, 1, nullptr).width);
        cv::Size size(width + 10, edge.label.size() * LINE_HEIGHT + 4);
        cv::Point center = edge.start + (edge.end - edge.start) * 0.5;
        edge.label_bounds = cv::Rect(center.x - size.width / 2,
            std::max(0, center.y - size.height / 2), size.width, size.height);
    }
}

std::string phase_label(const std::set<int>& phases)
{
    std::string result = "[";
    for (int phase : phases)
        result += (result.size() == 1 ? "" : ",") + std::to_string(phase);
    return result + "]";
}

}

KnowledgeGraphLayout make_knowledge_graph(const KnowledgeBase& knowledge_base, PredicateHandler& predicate_handler)
{
    KnowledgeGraphLayout graph;
    const auto& conclusions = knowledge_base.get_conclusions();
    const auto& facts = knowledge_base.get_facts();
    graph.evidence_count = facts.size();
    if (facts.empty())
        return graph;

    std::vector<int> fact_depths(facts.size(), 0);
    for (size_t i = 0; i < facts.size(); i++)
    {
        const auto& source = facts[i].get_source();
        fact_depths[i] = source.type == KnowledgeSourceType::GIVEN ? 0 : 1;
        for (int premise : source.source_indices)
        {
            if (premise < 0 || static_cast<size_t>(premise) >= i)
                throw std::logic_error("knowledge graph provenance must refer to earlier evidence");
            fact_depths[i] = std::max(fact_depths[i], fact_depths[premise] + 1);
        }
    }

    std::vector<size_t> fact_nodes(facts.size(), conclusions.size());
    std::vector<const KnowledgeEvidence*> fact_evidence(facts.size(), nullptr);
    std::vector<std::vector<size_t>> layers;
    for (size_t i = 0; i < conclusions.size(); i++)
    {
        const auto& conclusion = conclusions[i];
        if (conclusion.evidence.empty())
            throw std::logic_error("knowledge graph fact has no evidence");
        int depth = fact_depths.at(conclusion.evidence.front().fact_index);
        for (const auto& evidence : conclusion.evidence)
        {
            depth = std::min(depth, fact_depths.at(evidence.fact_index));
            fact_nodes.at(evidence.fact_index) = i;
            fact_evidence.at(evidence.fact_index) = &evidence;
        }
        KnowledgeGraphNode entry{i, depth, static_cast<int>(conclusion.evidence.size()), cv::Rect(), {}};
        for (size_t p = 0; p < conclusion.expression.predicates.size(); p++)
        {
            entry.lines.push_back("[" + std::to_string(conclusion.predicate_phases.at(p)) + "] "
                + predicate_handler.stringify_predicate(conclusion.expression.predicates[p]));
        }
        if (entry.lines.empty())
            entry.lines.push_back("(empty expression)");
        graph.nodes.push_back(std::move(entry));
        if (layers.size() <= static_cast<size_t>(depth))
            layers.resize(depth + 1);
        layers[depth].push_back(i);
    }

    for (size_t i = 0; i < facts.size(); i++)
    {
        const auto& source = facts[i].get_source();
        size_t node = fact_nodes[i];
        if (node >= graph.nodes.size() || fact_evidence[i] == nullptr)
            throw std::logic_error("knowledge graph evidence has no conclusion");
        if (source.type != KnowledgeSourceType::INFERRED)
            continue;

        graph.inference_count++;
        const auto& evidence = *fact_evidence[i];
        std::set<int> target_phases(evidence.predicate_phases.begin(), evidence.predicate_phases.end());
        const auto& phases = conclusions[node].phases;
        for (size_t phase = 0; phase < phases.size(); phase++)
        {
            if (phases[phase].fact_index == static_cast<int>(i))
            {
                target_phases = {static_cast<int>(phase + 1)};
                break;
            }
        }
        std::string proof = source.rule_name + " [K" + std::to_string(i) + "]";
        std::map<size_t, std::set<int>> premise_nodes;
        for (int premise : source.source_indices)
        {
            const auto& source_phases = fact_evidence.at(premise)->predicate_phases;
            auto& included = premise_nodes[fact_nodes[premise]];
            included.insert(source_phases.begin(), source_phases.end());
        }
        for (const auto& [premise, source_phases] : premise_nodes)
        {
            if (premise == node)
            {
                auto lines = wrap_text(proof + " uses " + phase_label(source_phases)
                    + " for " + phase_label(target_phases), MIN_NODE_WIDTH - 2 * PADDING);
                graph.nodes[node].lines.insert(graph.nodes[node].lines.end(), lines.begin(), lines.end());
                continue;
            }
            auto label = wrap_text(proof + " " + phase_label(source_phases) + " -> " + phase_label(target_phases),
                LAYER_GAP - 48, LABEL_SCALE);
            graph.edges.push_back(KnowledgeGraphEdge{premise, node, static_cast<int>(i), {}, {}, {}, label});
        }
        if (premise_nodes.empty())
        {
            auto lines = wrap_text(proof + " " + phase_label(target_phases) + " (no premises)",
                MIN_NODE_WIDTH - 2 * PADDING);
            graph.nodes[node].lines.insert(graph.nodes[node].lines.end(), lines.begin(), lines.end());
        }
    }
    std::vector<int> incoming(graph.nodes.size(), 0), outgoing(graph.nodes.size(), 0);
    for (const auto& edge : graph.edges)
    {
        incoming[edge.to]++;
        outgoing[edge.from]++;
    }
    for (size_t i = 0; i < graph.nodes.size(); i++)
    {
        auto& node = graph.nodes[i];
        std::string title = "C" + std::to_string(node.conclusion_index) + " | "
            + std::to_string(conclusions[node.conclusion_index].phases.size()) + " phases | "
            + std::to_string(node.evidence_count) + " sources";
        node.lines.insert(node.lines.begin(), title);
        int text_width = 0;
        for (const auto& line : node.lines)
            text_width = std::max(text_width,
                cv::getTextSize(line, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE, 1, nullptr).width);
        int text_height = 2 * PADDING + node.lines.size() * LINE_HEIGHT;
        int port_height = 2 * PADDING + 12 * (std::max(incoming[i], outgoing[i]) + 1);
        node.bounds = cv::Rect(0, 0, std::max(MIN_NODE_WIDTH, text_width + 2 * PADDING),
            std::max(text_height, port_height));
    }
    place_layers(graph, layers);
    connect_edges(graph);
    place_labels(graph);
    for (const auto& node : graph.nodes)
    {
        graph.size.width = std::max(graph.size.width, node.bounds.br().x + 2 * PADDING);
        graph.size.height = std::max(graph.size.height, node.bounds.br().y + 2 * PADDING);
    }
    for (const auto& edge : graph.edges)
    {
        graph.size.width = std::max(graph.size.width, edge.label_bounds.br().x + PADDING);
        graph.size.height = std::max(graph.size.height, edge.label_bounds.br().y + PADDING);
    }
    return graph;
}

cv::Rect knowledge_graph_viewport(cv::Size size)
{
    return cv::Rect(0, 115, size.width - 14, size.height - 155);
}

cv::Mat render_knowledge_graph(
    const KnowledgeGraphLayout& graph, cv::Size size, cv::Point2d& camera, double zoom)
{
    if (size.width < 160 || size.height < 180)
        throw std::invalid_argument("knowledge graph viewport is too small");
    if (!std::isfinite(zoom) || zoom <= 0)
        throw std::invalid_argument("knowledge graph zoom must be finite and positive");

    cv::Mat image(size, CV_8UC3, BACKGROUND);
    auto text = [&](const std::string& value, cv::Point point, double scale, cv::Scalar color)
    {
        cv::putText(image, value, point, cv::FONT_HERSHEY_SIMPLEX, scale, color, 1, cv::LINE_AA);
    };
    text("Knowledge base", cv::Point(18, 30), 0.8, TEXT);
    text(std::to_string(graph.nodes.size()) + " fact graphs / "
        + std::to_string(graph.inference_count) + " derivations", cv::Point(18, 55), 0.5, MUTED);
    text("C: shared fact    [n]: construction phase    K: proof (same K = joint premises)",
        cv::Point(18, 79), 0.45, MUTED);
    text("Facts", cv::Point(18, 103), 0.48, GIVEN);
    text("Inferred facts (columns: minimum proof depth)", cv::Point(100, 103), 0.48, INFERRED);
    text("Drag / arrows: pan    Wheel: zoom    " + std::to_string(cvRound(zoom * 100)) + "%",
        cv::Point(18, size.height - 10), 0.45, MUTED);

    cv::Rect viewport = knowledge_graph_viewport(size);
    cv::Mat body = image(viewport);
    cv::Point2d limit(std::max(0.0, graph.size.width * zoom - body.cols),
        std::max(0.0, graph.size.height * zoom - body.rows));
    camera.x = std::clamp(camera.x, 0.0, limit.x);
    camera.y = std::clamp(camera.y, 0.0, limit.y);
    if (graph.nodes.empty())
    {
        text("No knowledge yet. Enter a statement to populate the graph.",
            cv::Point(18, 145), 0.55, TEXT);
        return image;
    }
    auto project = [&](cv::Point point)
    {
        return cv::Point(cvRound(point.x * zoom - camera.x), cvRound(point.y * zoom - camera.y));
    };
    auto project_box = [&](const cv::Rect& box)
    {
        return cv::Rect(project(box.tl()), project(box.br()));
    };
    int thickness = std::max(1, cvRound(zoom));
    std::set<int> labeled_depths;
    for (const auto& node : graph.nodes)
    {
        if (labeled_depths.insert(node.depth).second)
        {
            cv::putText(body, node.depth == 0 ? "Given facts" : "Depth " + std::to_string(node.depth),
                project(cv::Point(node.bounds.x, 25)), cv::FONT_HERSHEY_SIMPLEX,
                0.5 * zoom, node.depth == 0 ? GIVEN : INFERRED, thickness, cv::LINE_AA);
        }
    }
    for (const auto& edge : graph.edges)
    {
        cv::Point start = project(edge.start);
        cv::Point end = project(edge.end);
        cv::line(body, start, end, EDGE, 1, cv::LINE_AA);
        cv::Point2d direction(edge.end - edge.start);
        direction *= 1.0 / cv::norm(direction);
        cv::Point2d perpendicular(-direction.y, direction.x);
        cv::line(body, end, cv::Point(cv::Point2d(end) - direction * (9 * zoom) + perpendicular * (4 * zoom)), EDGE, 1, cv::LINE_AA);
        cv::line(body, end, cv::Point(cv::Point2d(end) - direction * (9 * zoom) - perpendicular * (4 * zoom)), EDGE, 1, cv::LINE_AA);
    }
    for (const auto& edge : graph.edges)
    {
        cv::Rect box = project_box(edge.label_bounds);
        if ((box & cv::Rect(cv::Point(), body.size())).empty())
            continue;
        cv::rectangle(body, box, BACKGROUND, cv::FILLED);
        for (size_t i = 0; i < edge.label.size(); i++)
            cv::putText(body, edge.label[i], project(edge.label_bounds.tl() + cv::Point(5, 16 + i * LINE_HEIGHT)),
                cv::FONT_HERSHEY_SIMPLEX, LABEL_SCALE * zoom, TEXT, thickness, cv::LINE_AA);
    }
    for (const auto& node : graph.nodes)
    {
        cv::Rect box = project_box(node.bounds);
        if ((box & cv::Rect(cv::Point(), body.size())).empty())
            continue;
        cv::Scalar color = node.depth == 0 ? GIVEN : INFERRED;
        cv::rectangle(body, box, cv::Scalar(35, 31, 28), cv::FILLED);
        cv::rectangle(body, box, color, 1, cv::LINE_AA);
        for (size_t i = 0; i < node.lines.size(); i++)
            cv::putText(body, node.lines[i], project(node.bounds.tl() + cv::Point(PADDING, PADDING + 14 + i * LINE_HEIGHT)),
                cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE * zoom, i == 0 ? color : TEXT, thickness, cv::LINE_AA);
    }
    if (limit.y > 0)
    {
        int height = std::max(24, cvRound(body.rows * body.rows / (graph.size.height * zoom)));
        int top = viewport.y + cvRound(camera.y * (body.rows - height) / limit.y);
        cv::rectangle(image, cv::Rect(size.width - 9, viewport.y, 3, body.rows), cv::Scalar(60, 60, 60), cv::FILLED);
        cv::rectangle(image, cv::Rect(size.width - 11, top, 7, height), MUTED, cv::FILLED);
    }
    if (limit.x > 0)
    {
        int width = std::max(24, cvRound(body.cols * body.cols / (graph.size.width * zoom)));
        int left = cvRound(camera.x * (body.cols - width) / limit.x);
        cv::rectangle(image, cv::Rect(0, viewport.br().y + 5, body.cols, 3), cv::Scalar(60, 60, 60), cv::FILLED);
        cv::rectangle(image, cv::Rect(left, viewport.br().y + 3, width, 7), MUTED, cv::FILLED);
    }
    return image;
}
