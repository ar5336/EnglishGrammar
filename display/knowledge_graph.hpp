#ifndef KNOWLEDGE_GRAPH_HPP
#define KNOWLEDGE_GRAPH_HPP

#include <opencv2/core.hpp>
#include <string>
#include <vector>

#include "../logic/knowledge_base.hpp"

struct KnowledgeGraphNode
{
    size_t conclusion_index;
    int depth;
    int evidence_count = 0;
    cv::Rect bounds;
    std::vector<std::string> lines;
};

struct KnowledgeGraphEdge
{
    size_t from;
    size_t to;
    int fact_index;
    cv::Point start;
    cv::Point end;
    cv::Rect label_bounds;
    std::vector<std::string> label;
};

struct KnowledgeGraphLayout
{
    std::vector<KnowledgeGraphNode> nodes;
    std::vector<KnowledgeGraphEdge> edges;
    cv::Size size;
    size_t evidence_count = 0;
    size_t inference_count = 0;
};

struct KnowledgeGraphColors
{
    cv::Scalar background;
    cv::Scalar text;
    cv::Scalar given;
    cv::Scalar inferred;
    cv::Scalar highlight;
};

KnowledgeGraphLayout make_knowledge_graph(
    const KnowledgeBase& knowledge_base,
    PredicateHandler& predicate_handler);

cv::Rect knowledge_graph_viewport(cv::Size size);

cv::Mat render_knowledge_graph(
    const KnowledgeGraphLayout& graph,
    cv::Size size,
    cv::Point2d& camera,
    double zoom,
    const KnowledgeGraphColors& colors);

#endif
