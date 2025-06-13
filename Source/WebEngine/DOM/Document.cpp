#include "Document.hpp"
#include "CharacterData.hpp"

#include "Kori/Core.hpp"

#include <print>

namespace Hanami::DOM {

    void Document::print() const noexcept
    {
        static int32_t num_indents = -1;
        static bool exclude_empty_cdata = false;

        auto print_node = [&](this auto&& self, const Node* node) -> void
        {
            ++num_indents;
            KoriDefer { --num_indents; };

            auto indents = std::string{};
            for (int32_t i = 0; i < num_indents; i++)
                indents += '\t';

            if (const auto* cdata = dynamic_cast<const CharacterData*>(node))
            {
                if ((cdata->data() == " " || cdata->data() == "\n" || cdata->data() == "\t" || cdata->data() == "\f") && exclude_empty_cdata && node->children().empty())
                {
                    return;
                }
            }

            std::println("{}- {}:", indents, node_type_str(node->type()));

            indents += '\t';

            if (const auto* elem = dynamic_cast<const Element*>(node))
            {
                std::println("{}Namespace URI: {}", indents, elem->namespace_uri.value_or(""));
                std::println("{}Namespace Prefix: {}", indents, elem->namespace_prefix.value_or(""));
                std::println("{}Local Name: {}", indents, elem->local_name);
            }

            if (const auto* cdata = dynamic_cast<const CharacterData*>(node))
            {
                std::println("{}Data: {}", indents, cdata->data());
            }

            if (!node->children().empty())
            {
                std::println("{}Children:", indents);
                for (auto* child : node->children())
                {
                    self(child);
                }
            }
        };

        print_node(this);
    }

}
