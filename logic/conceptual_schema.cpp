#include "conceptual_schema.hpp"

ConceptualEntity::ConceptualEntity(string noun) : noun(noun)
{
    noun = "NOTHING";
}

void ConceptualSchema::print_maps()
{
    // print out the child_to_parents_map
    printf("CHILD TO PARENTS MAP\n");
    for (auto map_entry : child_to_parents_map)
    {
        string child = map_entry.first;
        set<string> parents = map_entry.second;

        printf("%s => [", child.c_str());
        for (string parent : parents)
        {
            printf("%s,", parent.c_str());
        }
        printf("]\n");
    }

    printf("PARENT TO CHILDREN MAP\n");
    for (auto map_entry : parent_to_children_map)
    {
        string parent = map_entry.first;
        set<string> children = map_entry.second;

        printf("%s => [", parent.c_str());
        for (string child : children)
        {
            printf("%s,", child.c_str());
        }
        printf("]\n");
    }
}

vector<pair<string, string>> ConceptualSchema::extract_inheritances(Expression expression)
{
    // IS (3) [noun_class] -> CONTAINS -> IS [noun_class]

    auto constructed_response = vector<pair<string, string>>();

    auto connection_pairs = expression.get_connections(
        "IS", "object",
        "CONTAINS", "container");

    if (connection_pairs.size() == 0)
        return constructed_response;

    for (auto connection_pair : connection_pairs)
    {
        Predicate is_predicate = connection_pair.first;

        if (!equals(is_predicate.get_argument("object_count"), "inf"))
        {
            // don't do this rule anymore
            return constructed_response;
        }

        Predicate contains_predicate = connection_pair.second;

        // auto parent_child_pairs = vector<tuple<string, string>>();

        auto second_connection_pairs = expression.get_connections(
            "CONTAINS", "containee",
            "IS", "object");

        if (second_connection_pairs.size() == 0)
            return constructed_response;
        
        for (auto second_connection_pair : second_connection_pairs)
        {
            Predicate other_is_predicate = second_connection_pair.second;

            string parent = other_is_predicate.get_argument("noun_class");
            string child = is_predicate.get_argument("noun_class");

            constructed_response.push_back(make_pair(parent, child));
            
            // child_to_parent.emplace(child, parent);
            // parent_child_pairs.push_back(make_tuple(parent, child));
        }
    }

    return constructed_response;
}

void ConceptualSchema::update_conceptual_maps(Expression new_expression)
{
    if (DEBUGGING)
        printf("\033[1;33mupdating\033[0m conceptual maps\n");
    
    apply_inheritance_rule(new_expression);
    apply_ability_rule(new_expression);
}

// returns bool is_resolved and a string message
pair<bool, string> ConceptualSchema::try_resolve_inquisitive_expression(Expression expression)
{
    // for now, only simple inheritance questions - "are dogs mammals", "are dogs animals"

    // first, check if the nouns_classes mentioned are defined in the schema
    auto expression_noun_set = expression.noun_set;
    for (auto expression_noun : expression_noun_set)
    {
        if (noun_class_set.size() == 0)
        {
            if (DEBUGGING)
                printf("no defined nouns\n");
            return make_pair(false, "no known nouns");
        }

        if (!has_noun(expression_noun))
        {   
            return make_pair(false, "noun '"+expression_noun+"' is not a known noun");
        }
    }

    if (DEBUGGING)
        printf("mentioned variables are known\n");

    // then, find what inheritance this expression indicates
    vector<pair<string, string>> inheritances = extract_inheritances(expression);

    bool constructed_bool = true;
    string constructed_response = "";

    if (DEBUGGING)
    {
        print_maps();
    }

    // build the response string as you consider each of the indicated inheritances
    if (DEBUGGING)
        printf("number of inheritances found: %ld\n", inheritances.size());

    if (inheritances.size() > 0)
    {
        for (auto inheritance : inheritances)
        {
            string child = inheritance.first;
            string parent = inheritance.second;

            if (child_to_parents_map.count(child) == 0)
            {
                constructed_bool &= false;
                constructed_response += ("no, "+child+" does not inherit from "+parent+".");
                continue;
            }
            set<string> potential_parent_matches = child_to_parents_map.at(child);

            if (potential_parent_matches.find(parent) != potential_parent_matches.end())
            {
                constructed_response += ("yes, "+child+" inherits from "+parent+".");
            } else {

                constructed_response += ("no, "+child+" does not inherit from "+parent+".");
            }
        }
        return make_pair(true, constructed_response);
    }
    

    vector<pair<string, string>> abilities = extract_abilities(expression);

    if (abilities.size() > 0)
    {
        for (auto ability_pair : abilities)
        {
            string noun = ability_pair.first;
            string ability = ability_pair.second;

            if (DEBUGGING)
                printf("checking if %s can do  ability:'%s'", noun.c_str(), ability.c_str());

            if (can_do(noun, ability))
            {
                return make_pair(true, "yes, "  + noun + " can " + ability + ".");
            }
            else
            {
                return make_pair(true, "no, " + noun + " can not " + ability + ".");
            }
        }
    }
 

    return make_pair(false, "");
}

ConceptualSchema::ConceptualSchema()
{
    noun_class_set = set<string>();
    entities_by_noun = map<string, ConceptualEntity>();
}

bool ConceptualSchema::has_noun(string noun)
{
    return noun_class_set.count(noun) != 0;
}

ConceptualEntity ConceptualSchema::get_noun_entity(string noun)
{
    if (!has_noun(noun))
        throw runtime_error("noun of class name \'" + noun + "\' not found in conceptual schema");

    return entities_by_noun.at(noun);
}

void ConceptualSchema::add_entity(ConceptualEntity new_node)
{
    if (DEBUGGING)
    {
        printf("\033[1;32madding\033[0m object of noun class '%s'\n", new_node.noun.c_str());
    }
    entities_by_noun.emplace(new_node.noun, new_node);
}

void ConceptualSchema::consider_expression(Expression expression)
{
    auto expression_nouns = expression.noun_set;

    set<string> new_nouns = set<string>();

    for (string expression_noun : expression_nouns)
    {
        if (!has_noun(expression_noun))
        {
            if (DEBUGGING)
                printf("added new noun '%s' to noun_class_set\n", expression_noun.c_str());
            // if expression noun is not present in current nouns
            noun_class_set.insert(expression_noun);
            add_entity(ConceptualEntity(expression_noun));
        }
    }

    update_conceptual_maps(expression);
    // make_inferences(expression);
}

void ConceptualSchema::apply_ability_rule(Expression expression)
{
    if (DEBUGGING)
        printf("applying ability rule\n");

    auto noun_ability_pairs = extract_abilities(expression);

    if (DEBUGGING && noun_ability_pairs.size() > 0)
        printf("%d abilities found in expression\n", (int)noun_ability_pairs.size());

    for (auto noun_ability_pair : noun_ability_pairs)
    {
        string actor_arg = noun_ability_pair.first;
        string ability_arg = noun_ability_pair.second;

        if (ability_map.count(actor_arg) == 0)
        {
            add_ability(actor_arg, ability_arg);

            update_abilities(actor_arg, ability_arg);
        }
        else {
            set<string> current_abilities = ability_map.at(actor_arg);
            if (current_abilities.count(ability_arg) == 0)
            {
                add_ability(actor_arg, ability_arg);
                ability_map.at(actor_arg).emplace(ability_arg);
                update_abilities(actor_arg, ability_arg);
            }
            // can already do ability
        }

    }

}

void ConceptualSchema::add_ability(string noun, string action)
{
    if (DEBUGGING)
        printf("\033[1;32madding\033[0m ability '%s' to noun '%s'\n", action.c_str(), noun.c_str());

    if (ability_map.count(noun) == 0)
    {
        ability_map.emplace(noun, set<string>{action});
    } else {
        ability_map.at(noun).emplace(action);
    }
}

vector<pair<string, string>> ConceptualSchema::extract_abilities(Expression expression)
{
    auto actor_ability_pairs = expression.get_connections(
        "IS", "object",
        "CAN_DO", "actor");

    if (DEBUGGING)
        printf("extracted %d abilities\n", (int)actor_ability_pairs.size());

    vector<pair<string, string>> noun_ability_pairs = vector<pair<string, string>>();
    for (auto actor_ability_pair : actor_ability_pairs)
    {
        Predicate actor_predicate = actor_ability_pair.first;
        Predicate ability_predicate = actor_ability_pair.second;

        string actor_arg = actor_predicate.get_argument("noun_class");
        string ability_arg = ability_predicate.get_argument("action_type");

        noun_ability_pairs.push_back(make_pair(actor_arg, ability_arg));

    }

    return noun_ability_pairs;
}


void ConceptualSchema::apply_inheritance_rule(Expression expression)
{
    int LOOP_LIMIT = 3;

    if (DEBUGGING)
        printf("applying inheritance rule\n");

    auto inheritances = extract_inheritances(expression);

    // add the map connections
    for (auto inheritance : inheritances)
    {
        auto child = inheritance.first;
        auto parent = inheritance.second;

        if (child_to_parents_map.count(child) == 0)
        {
            child_to_parents_map.emplace(child, set<string>{parent});
        }
        else 
        {
            child_to_parents_map.at(child).emplace(parent);
        }

        if (parent_to_children_map.count(parent) == 0)
        {
            parent_to_children_map.emplace(parent, set<string>{child});
        }
        else
        {
            parent_to_children_map.at(parent).emplace(child);
        }

        if (DEBUGGING)
            printf("new inheritance added from child:'%s' to parent:'%s'\n", child.c_str(), parent.c_str());

        update_inheritances(child, parent);
    }
    
    return;

    // for each new parent child pair, create the 
}

void ConceptualSchema::update_inheritances(string child, string parent)
{
    // noun is expected to have recieved a new parent, update all of its children
    // using the child to parents and parent to chlidren map, update them both.
    if (DEBUGGING)
        printf("updating inheritance maps for child '%s' and parent '%s'\n", child.c_str(), parent.c_str());
    
    if (DEBUGGING)
    {
        printf("the maps as they are whilst updating:\n");
        print_maps();
    }

    // TODO - put a update_abilities() call in some places here.
    // for the case of | mammals are animals, animals can swim, dogs are mammals ==> dogs can swim

    // step 1) make sure all the parents of the parent are inherited to the child
    if (child_to_parents_map.count(parent) != 0)
    {
        // so, in this case it would be that child is cat and parent is animal
        set<string> parents_parents = child_to_parents_map.at(parent);
        for (string grandparent : parents_parents)
        {
            // child: 
            child_to_parents_map.at(child).insert(grandparent);
            parent_to_children_map.at(grandparent).insert(child);
            if (DEBUGGING)
                printf("\033[1;32madding\033[0m grandparent '%s' to parents of child '%s\n", grandparent.c_str(), child.c_str());
        }
    }

    if (parent_to_children_map.count(child) == 0)
        return;
        // throw runtime_error("could not find child '" + child + "' in parent_to_children map");


    //step 2) make sure the parents are inherited to all children
    auto children_visited = set<string>();
    vector<string> children_to_traverse = vector<string>{child};

    while (children_to_traverse.size() != 0)
    {
        string child_to_traverse = children_to_traverse.back();
        children_to_traverse.pop_back();

        auto children_of_child = parent_to_children_map.at(child_to_traverse);

        for (string child_of_child : children_of_child)
        {
            if (child_to_parents_map.count(child_of_child) == 0)
                throw runtime_error("nested child "+child_of_child+" not found in child_to_parent map");
            
            set<string> sub_parents = child_to_parents_map.at(child_of_child);
            
            if (sub_parents.find(parent) == sub_parents.end())
            {
                child_to_parents_map.at(child_of_child).emplace(parent);

                // if this child you just updated the parents of, has children of its own, look at it later.
                if (parent_to_children_map.count(child_of_child) != 0
                    && children_visited.find(child_of_child) == children_visited.end())
                {
                    if (DEBUGGING)
                        printf("\033[1;32madding\033[0m child '%s' to traversal\n", child_of_child.c_str());
                    children_to_traverse.push_back(child_of_child);
                }
                children_visited.emplace(child_to_traverse);

                if (DEBUGGING)
                {
                    printf("\033[1;32madded\033[0m parent '%s' to child '%s'\n", parent.c_str(), child_of_child.c_str());
                }
            }
        }
    }
}

void ConceptualSchema::update_abilities(string noun, string ability, int recursion)
{
    int ABILITY_RECURSION_DEPTH = 4;

    if (recursion > ABILITY_RECURSION_DEPTH)
    {
        printf("WARNING: recursion depth limit reached in update_abilities");
        return;
    }

    if (parent_to_children_map.count(noun) == 0)
    {
        return;
    }

    set<string> children = parent_to_children_map.at(noun);
    
    for (string child : children)
    {
        if (!can_do(child, ability))
        {
            add_ability(child, ability);
            update_abilities(child, ability, recursion + 1);
        }
    }
}

bool ConceptualSchema::can_do(string noun, string action)
{
    if (DEBUGGING)
        printf("checking if noun '%s' can do action '%s'\n", noun.c_str(), action.c_str());
    
    return (ability_map.count(noun) != 0) &&
        (ability_map.at(noun).count(action) != 0);
}