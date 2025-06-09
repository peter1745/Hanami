#include "tree_builder.hpp"

#include <csignal>
#include <print>

#include "kori/core.hpp"

#define TODO(...) raise(SIGTRAP)
//#define TODO(...)

namespace hanami::html {

    auto Node::first_child() const noexcept -> Node*
    {
        if (m_child_nodes.empty())
        {
            return nullptr;
        }

        return m_child_nodes.front();
    }

    auto Node::last_child() const noexcept -> Node*
    {
        if (m_child_nodes.empty())
        {
            return nullptr;
        }

        return m_child_nodes.back();
    }

    // https://dom.spec.whatwg.org/#concept-node-pre-insert
    auto Node::insert_before(Node* node, Node* child) -> Node*
    {
        // 1. TODO: Ensure pre-insert validity of node into parent before child.
        // 2. Let referenceChild be child.
        auto* reference_child = child;

        // 3. If referenceChild is node, then set referenceChild to node’s next sibling.
        if (reference_child == node)
        {
            reference_child = node->m_next_sibling;
        }

        // 4. Insert node into parent before referenceChild.
        auto reference_child_pos = std::ranges::find(m_child_nodes, reference_child);
        m_child_nodes.insert(reference_child_pos, node);

        // FIXME(Peter): Hack around using the proper insertion steps.
        node->m_document = m_type == NodeType::Document ? dynamic_cast<Document*>(this) : m_document;
        node->m_parent = this;

        // 5. Return node.
        return node;
    }

    // https://dom.spec.whatwg.org/#concept-node-append
    auto Node::append_child(Node* node) -> Node*
    {
        return insert_before(node, nullptr);
    }

    TreeBuilder::TreeBuilder() noexcept
    {
        m_document = std::make_unique<Document>();
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#tree-construction
    void TreeBuilder::process_all_tokens(std::span<const Token> tokens)
    {
        size_t current_token_idx = 0;

        auto consume_token = [&] -> const Token&
        {
            return tokens[current_token_idx++];
        };

        while (current_token_idx < tokens.size())
        {
            const auto& token = consume_token();

            auto is_special_character_token = [&] -> bool
            {
                // Character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                auto* c = std::get_if<CharacterToken>(&token);
                return c && (c->data == '\t' || c->data == '\n' || c->data == '\f' || c->data == '\r' || c->data == ' ');
            };

            if (
                // If the stack of open elements is empty
                m_open_elements.empty() ||

                // If the adjusted current node is an element in the HTML namespace
                adjusted_current_node()->is_in_namespace(html_namespace) ||

                // TODO(Peter): MathML + HTML integration points
                // If the adjusted current node is a MathML text integration point and the token is a start tag whose tag name is neither "mglyph" nor "malignmark"
                // If the adjusted current node is a MathML text integration point and the token is a character token
                // If the adjusted current node is a MathML annotation-xml element and the token is a start tag whose tag name is "svg"
                // If the adjusted current node is an HTML integration point and the token is a start tag
                // If the adjusted current node is an HTML integration point and the token is a character token

                // If the token is an end-of-file token
                token_is<EOFToken>(token)
            )
            {
                // Process the token according to the rules given in the section corresponding to the current insertion mode in HTML content.

                switch (m_insertion_mode)
                {
                    // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
                    case TreeInsertionMode::Initial:
                    {
                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (is_special_character_token())
                        {
                            // Ignore the token.
                            break;
                        }

                        // A comment token
                        if (auto* c = std::get_if<CommentToken>(&token); c)
                        {
                            // Insert a comment as the last child of the Document object.
                            insert_comment(c->data, NodeListLocation{ &m_document->m_child_nodes, m_document->m_child_nodes.end() });
                            break;
                        }

                        // A DOCTYPE token
                        if (auto* d = std::get_if<DOCTYPEToken>(&token); d)
                        {
                            // If the DOCTYPE token's name is not "html",
                            // or the token's public identifier is not missing,
                            // or the token's system identifier is neither missing nor "about:legacy-compat",
                            // then there is a parse error.

                            if (d->name != "html" ||
                                d->public_identifier.has_value() ||
                                (d->system_identifier.has_value() && d->system_identifier.value() != "about:legacy-compat"))
                            {
                                // Report parse error
                            }

                            // Append a DocumentType node to the Document node,
                            // with its name set to the name given in the DOCTYPE token, or the empty string if the name was missing;
                            // its public ID set to the public identifier given in the DOCTYPE token, or the empty string if the public identifier was missing;
                            // and its system ID set to the system identifier given in the DOCTYPE token, or the empty string if the system identifier was missing.

                            m_document->append_child(new DocumentType(
                                d->name,
                                d->public_identifier.value_or(""),
                                d->system_identifier.value_or("")));

                            // TODO: If this becomes relevant
                            // Then, if the document is not an iframe srcdoc document,
                            // and the parser cannot change the mode flag is false,
                            // and the DOCTYPE token matches one of the conditions in the following list,
                            // then set the Document to quirks mode:

                            // TODO: If this becomes relevant
                            // Otherwise, if the document is not an iframe srcdoc document,
                            // and the parser cannot change the mode flag is false,
                            // and the DOCTYPE token matches one of the conditions in the following list,
                            // then set the Document to limited-quirks mode:

                            // The system identifier and public identifier strings must be compared to the values given in the lists above in an ASCII case-insensitive manner.
                            // A system identifier whose value is the empty string is not considered missing for the purposes of the conditions above.

                            // Then, switch the insertion mode to "before html".
                            m_insertion_mode = TreeInsertionMode::BeforeHTML;
                            break;
                        }

                        // If the document is not an iframe srcdoc document, then this is a parse error;
                        // if the parser cannot change the mode flag is false, set the Document to quirks mode.

                        // In any case, switch the insertion mode to "before html", then reprocess the token.
                        m_insertion_mode = TreeInsertionMode::BeforeHTML;
                        --current_token_idx;
                        break;
                    }
                    case TreeInsertionMode::BeforeHTML:
                    {
                        // A DOCTYPE token
                        if (auto* d = std::get_if<DOCTYPEToken>(&token); d)
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A comment token
                        if (auto* c = std::get_if<CommentToken>(&token); c)
                        {
                            // Insert a comment as the last child of the Document object.
                            insert_comment(c->data, NodeListLocation{ &m_document->m_child_nodes, m_document->m_child_nodes.end() });
                            break;
                        }

                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (is_special_character_token())
                        {
                            // Ignore the token.
                            break;
                        }

                        // A start tag whose tag name is "html"
                        if (auto* t = std::get_if<StartTagToken>(&token); t && t->name == "html")
                        {
                            // Create an element for the token in the HTML namespace, with the Document as the intended parent.
                            auto* element = create_element_for_token(token, html_namespace, m_document.get());

                            // Append it to the Document object.
                            m_document->append_child(element);

                            // Put this element in the stack of open elements.
                            m_open_elements.emplace_back(element);

                            // Switch the insertion mode to "before head".
                            m_insertion_mode = TreeInsertionMode::BeforeHead;
                            break;
                        }

                        // An end tag
                        if (auto* t = std::get_if<EndTagToken>(&token); t)
                        {
                            // whose tag name is one of: "head", "body", "html", "br"
                            if (t->name == "head" || t->name == "body" || t->name == "html" || t->name == "br")
                            {
                                // Act as described in the "anything else" entry below.
                            }
                            else
                            {
                                // Any other end tag
                                // Parse error. Ignore the token.
                                break;
                            }
                        }

                        // Anything else
                        // Create an html element whose node document is the Document object.
                        auto* element = new HTMLHtmlElement();
                        element->m_document = m_document.get();

                        // Append it to the Document object.
                        m_document->append_child(element);

                        // Put this element in the stack of open elements.
                        m_open_elements.emplace_back(element);

                        // Switch the insertion mode to "before head", then reprocess the token.
                        m_insertion_mode = TreeInsertionMode::BeforeHead;

                        --current_token_idx;

                        // The document element can end up being removed from the Document object, e.g. by scripts;
                        // nothing in particular happens in such cases, content continues being appended to the nodes as described in the next section.
                        break;
                    }
                    case TreeInsertionMode::BeforeHead:
                    {
                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (is_special_character_token())
                        {
                            // Ignore the token.
                            break;
                        }

                        // A comment token
                        if (auto* c = std::get_if<CommentToken>(&token); c)
                        {
                            // Insert a comment.
                            insert_comment(c->data);
                            break;
                        }

                        // A DOCTYPE token
                        if (auto* d = std::get_if<DOCTYPEToken>(&token); d)
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A start tag
                        if (auto* t = std::get_if<StartTagToken>(&token); t)
                        {
                            // whose tag name is "html"
                            if (t->name == "html")
                            {
                                // Process the token using the rules for the "in body" insertion mode.
                                TODO();
                                break;
                            }

                            // whose tag name is "head"
                            if (t->name == "head")
                            {
                                // Insert an HTML element for the token.
                                auto* head = insert_html_element(token);

                                // Set the head element pointer to the newly created head element.
                                m_document->m_head = head;

                                // Switch the insertion mode to "in head".
                                m_insertion_mode = TreeInsertionMode::InHead;
                                break;
                            }
                        }

                        // An end tag
                        if (auto* t = std::get_if<EndTagToken>(&token); t)
                        {
                            // whose tag name is one of: "head", "body", "html", "br"
                            if (t->name == "head" || t->name == "body" || t->name == "html" || t->name == "br")
                            {
                                // Act as described in the "anything else" entry below.
                            }
                            else
                            {
                                // Any other end tag
                                // Parse error. Ignore the token.
                                break;
                            }
                        }


                        // Anything else
                        // Insert an HTML element for a "head" start tag token with no attributes.
                        auto* head = insert_html_element(token);

                        // Set the head element pointer to the newly created head element.
                        m_document->m_head = head;

                        // Switch the insertion mode to "in head".
                        m_insertion_mode = TreeInsertionMode::InHead;

                        // Reprocess the current token.
                        --current_token_idx;
                        break;
                    }
                    case TreeInsertionMode::InHead:
                    {
                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (is_special_character_token())
                        {
                            // Insert the character.
                            const auto& [data] = std::get<CharacterToken>(token);
                            insert_character({ &data, 1 });
                            break;
                        }

                        // A comment token
                        if (const auto* c = std::get_if<CommentToken>(&token); c)
                        {
                            // Insert a comment.
                            insert_comment(c->data);
                            break;
                        }

                        // A DOCTYPE token
                        if (std::get_if<DOCTYPEToken>(&token))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A start tag
                        if (auto* t = std::get_if<StartTagToken>(&token); t)
                        {
                            // whose tag name is "html"
                            if (t->name == "html")
                            {
                                // Process the token using the rules for the "in body" insertion mode.
                                TODO();
                                break;
                            }

                            // whose tag name is one of: "base", "basefont", "bgsound", "link"
                            if (t->name == "base" || t->name == "basefont" || t->name == "bgsound" || t->name == "link")
                            {
                                // Insert an HTML element for the token. Immediately pop the current node off the stack of open elements.
                                // Acknowledge the token's self-closing flag, if it is set.
                                TODO();
                                break;
                            }

                            // whose tag name is "meta"
                            if (t->name == "meta")
                            {
                                // Insert an HTML element for the token. Immediately pop the current node off the stack of open elements.
                                insert_html_element(token);

                                m_open_elements.pop_back();

                                // Acknowledge the token's self-closing flag, if it is set.
                                std::visit(kori::VariantOverloadSet {
                                   [](const EndTagToken& tag)
                                   {
                                       if (tag.self_closing)
                                       {
                                           // parse_error(ErrorType::ParseErrorWithTrailingSolidus);
                                       }
                                   },
                                   [](auto&&)
                                   {
                                   }
                               }, token);

                                // If the active speculative HTML parser is null, then:
                                //if (speculative_parser_null)
                                //{
                                // If the element has a charset attribute,
                                // and getting an encoding from its value results in an encoding,
                                // and the confidence is currently tentative,
                                // then change the encoding to the resulting encoding.

                                // Otherwise, if the element has an http-equiv attribute whose value is an ASCII case-insensitive match for the string "Content-Type",
                                // and the element has a content attribute,
                                // and applying the algorithm for extracting a character encoding from a meta element to that attribute's value returns an encoding,
                                // and the confidence is currently tentative,
                                // then change the encoding to the extracted encoding.
                                //}
                                break;
                            }

                            // whose tag name is "title"
                            if (t->name == "title")
                            {
                                // Follow the generic RCDATA element parsing algorithm.
                                TODO("This requires implementing proper tokenizer - tree builder intero. E.g emitted tokens immediately get processed by the tree construction. Do this next.");
                                break;
                            }

                            if (
                                // whose tag name is "noscript", if the scripting flag is enabled
                                (t->name == "noscript" && m_document->m_scripting) ||
                                // whose tag name is one of: "noframes", "style"
                                (t->name == "noframes" || t->name == "style")
                            )
                            {
                                // TODO(Peter):
                                // Follow the generic raw text element parsing algorithm.
                                TODO();
                                break;
                            }

                            // whose tag name is "noscript", if the scripting flag is disabled
                            if (t->name == "noscript" && !m_document->m_scripting)
                            {
                                // TODO(Peter):
                                // Insert an HTML element for the token.
                                // Switch the insertion mode to "in head noscript".
                                TODO();
                                break;
                            }

                            if (t->name == "script")
                            {
                                TODO();
                                break;
                            }

                            // whose tag name is "template"
                            if (t->name == "template")
                            {
                                TODO();
                            }
                        }

                        // An end tag
                        if (auto* t = std::get_if<EndTagToken>(&token); t)
                        {
                            // whose tag name is "head"
                            if (t->name == "head")
                            {
                                // Pop the current node (which will be the head element) off the stack of open elements.
                                m_open_elements.pop_back();

                                // Switch the insertion mode to "after head".
                                m_insertion_mode = TreeInsertionMode::AfterHead;
                                break;
                            }

                            // whose tag name is one of: "body", "html", "br"
                            if (t->name == "body" || t->name == "html" || t->name == "br")
                            {
                                // Act as described in the "anything else" entry below.

                                // Pop the current node (which will be the head element) off the stack of open elements.
                                m_open_elements.pop_back();

                                // Switch the insertion mode to "after head".
                                m_insertion_mode = TreeInsertionMode::AfterHead;

                                // Reprocess the token.
                                --current_token_idx;
                                break;
                            }

                            // whose tag name is "template"
                            if (t->name == "template")
                            {
                                TODO();
                            }
                        }

                        // A start tag whose tag name is "head"
                        // Any other end tag
                        if (auto* t = std::get_if<StartTagToken>(&token); (t && t->name == "head") || std::get_if<EndTagToken>(&token))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // Anything else
                        // Pop the current node (which will be the head element) off the stack of open elements.
                        m_open_elements.pop_back();

                        // Switch the insertion mode to "after head".
                        m_insertion_mode = TreeInsertionMode::AfterHead;

                        // Reprocess the token.
                        --current_token_idx;
                        break;
                    }
                    default:
                    {
                        TODO();
                        break;
                    }
                }

            }
            else // Otherwise
            {
                // Process the token according to the rules given in the section for parsing tokens in foreign content.
            }

            // The next token is the token that is about to be processed by the tree construction dispatcher (even if the token is subsequently just ignored).

            // A node is a MathML text integration point if it is one of the following elements:
            // A MathML mi element
            // A MathML mo element
            // A MathML mn element
            // A MathML ms element
            // A MathML mtext element

            // A node is an HTML integration point if it is one of the following elements:
            // A MathML annotation-xml element whose start tag token had an attribute with the name "encoding" whose value was an ASCII case-insensitive match for the string "text/html"
            // A MathML annotation-xml element whose start tag token had an attribute with the name "encoding" whose value was an ASCII case-insensitive match for the string "application/xhtml+xml"
            // An SVG foreignObject element
            // An SVG desc element
            // An SVG title element
        }
    }

    auto TreeBuilder::current_node() const noexcept -> Element*
    {
        // The current node is the bottommost node in this stack of open elements.

        if (m_open_elements.empty())
        {
            return nullptr;
        }

        return m_open_elements.back();
    }

    auto TreeBuilder::adjusted_current_node() const noexcept -> Element*
    {
        // The adjusted current node is the context element if the parser was created as
        // part of the HTML fragment parsing algorithm and the stack of open elements has
        // only one element in it (fragment case); otherwise, the adjusted current node is the current node.
        return current_node();
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#appropriate-place-for-inserting-a-node
    auto TreeBuilder::appropriate_insertion_place(std::optional<Node*> override_target) const noexcept -> NodeListLocation
    {
        // 1. If there was an override target specified, then let target be the override target.
        //    Otherwise, let target be the current node.
        auto* target = override_target.value_or(current_node());

        // 2. Determine the adjusted insertion location using the first matching steps from the following list:
        //    If foster parenting is enabled and target is a table, tbody, tfoot, thead, or tr element
        //      Run these substeps:
        //          Let last template be the last template element in the stack of open elements, if any.
        //          Let last table be the last table element in the stack of open elements, if any.
        //          If there is a last template and either there is no last table, or there is one, but last template is lower (more recently added) than last table in the stack of open elements, then: let adjusted insertion location be inside last template's template contents, after its last child (if any), and abort these steps.
        //          If there is no last table, then let adjusted insertion location be inside the first element in the stack of open elements (the html element), after its last child (if any), and abort these steps. (fragment case)
        //          If last table has a parent node, then let adjusted insertion location be inside last table's parent node, immediately before last table, and abort these steps.
        //          Let previous element be the element immediately above last table in the stack of open elements.
        //          Let adjusted insertion location be inside previous element, after its last child (if any).

        // Otherwise
        //  Let adjusted insertion location be inside target, after its last child (if any).
        auto adjusted_insertion_location = NodeListLocation{ &target->m_child_nodes, target->m_child_nodes.end() };

        // 3. If the adjusted insertion location is inside a template element,
        // let it instead be inside the template element's template contents, after its last child (if any).
        // 4. Return the adjusted insertion location.
        return adjusted_insertion_location;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#insert-a-character
    void TreeBuilder::insert_character(std::string_view data) noexcept
    {
        // Let data be the characters passed to the algorithm, or, if no characters were explicitly specified, the character of the character token being processed.
        // Let the adjusted insertion location be the appropriate place for inserting a node.
        auto adjusted_insertion_location = appropriate_insertion_place();

        // If the adjusted insertion location is in a Document node, then return.
        // The DOM will not let Document nodes have Text node children, so they are dropped on the floor.
        if (dynamic_cast<Document*>(current_node()) != nullptr)
        {
            // NOTE(Peter): Filthy dynamic_cast
            return;
        }

        // If there is a Text node immediately before the adjusted insertion location, then append data to that Text node's data.
        /*Node* before = nullptr;

        if (adjusted_insertion_location == current_node()->m_child_nodes.end())
        {
            before = nullptr;
        }
        else if (adjusted_insertion_location != current_node()->m_child_nodes.begin())
        {
            before = *(adjusted_insertion_location--);
        }*/

        if (auto* t = dynamic_cast<Text*>(*adjusted_insertion_location); t)
        {
            t->m_data += data;
        }
        else
        {
            // Otherwise, create a new Text node whose data is data and
            // whose node document is the same as that of the element in which the adjusted insertion location finds itself,
            // and insert the newly created node at the adjusted insertion location.
            auto* text = new Text(data);
            text->m_document = current_node()->m_document;
            current_node()->insert_before(text, *adjusted_insertion_location);
        }
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#insert-a-comment
    void TreeBuilder::insert_comment(std::string_view data, std::optional<NodeListLocation> pos) noexcept
    {
        // Let data be the data given in the comment token being processed.
        // If position was specified, then let the adjusted insertion location be position. Otherwise, let adjusted insertion location be the appropriate place for inserting a node.
        auto adjusted_insertion_location = pos.value_or(appropriate_insertion_place());

        // Create a Comment node whose data attribute is set to data
        // and whose node document is the same as that of the node in which the adjusted insertion location finds itself.
        auto comment = new Comment(data);
        comment->m_document = current_node()->m_document;

        // Insert the newly created node at the adjusted insertion location.
        current_node()->insert_before(comment, *adjusted_insertion_location);
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#insert-an-element-at-the-adjusted-insertion-location
    void TreeBuilder::insert_element_at_adjusted_insertion_location(Element* element, NodeListLocation adjusted_insertion_location)
    {
        // Let the adjusted insertion location be the appropriate place for inserting a node.
        // If it is not possible to insert element at the adjusted insertion location, abort these steps.
        if (auto* d = dynamic_cast<Document*>(current_node()); d && d->first_child()->is_element())
        {
            return;
        }

        // TODO(Peter):
        // If the parser was not created as part of the HTML fragment parsing algorithm,
        // then push a new element queue onto element's relevant agent's custom element reactions stack.

        // Insert element at the adjusted insertion location.
        current_node()->insert_before(element, *adjusted_insertion_location);

        // If the parser was not created as part of the HTML fragment parsing algorithm,
        // then pop the element queue from element's relevant agent's custom element reactions stack,
        // and invoke custom element reactions in that queue.
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#insert-an-html-element
    auto TreeBuilder::insert_html_element(const Token& token) -> Element*
    {
        return insert_foreign_element(token, html_namespace, false);
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
    auto TreeBuilder::insert_foreign_element(const Token& token, std::string_view element_namespace, bool only_add_to_element_stack) -> Element*
    {
        // 1. Let the adjustedInsertionLocation be the appropriate place for inserting a node.
        auto adjusted_insertion_location = appropriate_insertion_place();

        // 2. Let element be the result of creating an element for the token given token, namespace, and the element in which the adjustedInsertionLocation finds itself.
        auto* element = create_element_for_token(token, element_namespace, current_node());

        // 3. If onlyAddToElementStack is false, then run insert an element at the adjusted insertion location with element.
        if (!only_add_to_element_stack)
        {
            insert_element_at_adjusted_insertion_location(element, adjusted_insertion_location);
        }

        // 4. Push element onto the stack of open elements so that it is the new current node.
        m_open_elements.emplace_back(element);

        // 5. Return element.
        return element;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
    auto TreeBuilder::create_element_for_token(const Token& token, std::string_view element_namespace, Node* intended_parent) -> Element*
    {
        const TagToken* tag_token = nullptr;
        std::visit(kori::VariantOverloadSet{
                       [&](const StartTagToken& tag) { tag_token = &tag; },
                       [&](const EndTagToken& tag) { tag_token = &tag; },
                       [](auto&&) { raise(SIGTRAP); }
                   }, token);

        // TODO(Peter): Speculative parser
        // If the active speculative HTML parser is not null, then return the result of creating a speculative mock element given namespace, token's tag name, and token's attributes.
        // Otherwise, optionally create a speculative mock element given namespace, token's tag name, and token's attributes.
        // The result is not used. This step allows for a speculative fetch to be initiated from non-speculative parsing. The fetch is still speculative at this point, because, for example, by the time the element is inserted, intended parent might have been removed from the document.

        // Let document be intendedParent's node document.
        auto* document = intended_parent->m_document;

        // Let localName be token's tag name.
        std::string_view local_name = tag_token->name;

        // Let is be the value of the "is" attribute in token, if such an attribute exists; otherwise null.
        auto is = get_token_attribute_value(tag_token, "is");

        // TODO(Peter): Custom Registry
        // Let registry be the result of looking up a custom element registry given intendedParent.
        // Let definition be the result of looking up a custom element definition given registry, namespace, localName, and is.
        void* definition = nullptr;

        // Let willExecuteScript be true if definition is non-null and the parser was not created as part of the HTML fragment parsing algorithm; otherwise false.
        bool will_execute_script = definition != nullptr;

        // If willExecuteScript is true:
        if (will_execute_script)
        {
            // Increment document's throw-on-dynamic-markup-insertion counter.
            // If the JavaScript execution context stack is empty, then perform a microtask checkpoint.
            // Push a new element queue onto document's relevant agent's custom element reactions stack.
            TODO();
        }

        // Let element be the result of creating an element given document, localName, namespace, null, is, willExecuteScript, and registry.
        // This will cause custom element constructors to run, if willExecuteScript is true. However, since we incremented the throw-on-dynamic-markup-insertion counter, this cannot cause new characters to be inserted into the tokenizer, or the document to be blown away.
        auto* element = create_element(document, local_name, element_namespace, std::nullopt, is, will_execute_script);

        // TODO(Peter):
        // Append each attribute in the given token to element.
        // This can enqueue a custom element callback reaction for the attributeChangedCallback, which might run immediately (in the next step).
        // Even though the is attribute governs the creation of a customized built-in element, it is not present during the execution of the relevant custom element constructor; it is appended in this step, along with all other attributes.

        // If willExecuteScript is true:
        if (will_execute_script)
        {
            TODO();
            // Let queue be the result of popping from document's relevant agent's custom element reactions stack. (This will be the same element queue as was pushed above.)
            // Invoke custom element reactions in queue.
            // Decrement document's throw-on-dynamic-markup-insertion counter.
        }

        // TODO(Peter):
        // If element has an xmlns attribute in the XMLNS namespace whose value is not exactly the same as the element's namespace, that is a parse error. Similarly, if element has an xmlns:xlink attribute in the XMLNS namespace whose value is not the XLink Namespace, that is a parse error.

        // TODO(Peter):
        // If element is a resettable element and not a form-associated custom element, then invoke its reset algorithm. (This initializes the element's value and checkedness based on the element's attributes.)

        // TODO(Peter):
        // If element is a form-associated element and not a form-associated custom element, the form element pointer is not null, there is no template element on the stack of open elements, element is either not listed or doesn't have a form attribute, and the intendedParent is in the same tree as the element pointed to by the form element pointer, then associate element with the form element pointed to by the form element pointer and set element's parser inserted flag.

        // Return element.
        return element;
    }

    // https://dom.spec.whatwg.org/#concept-create-element
    auto TreeBuilder::create_element(
        Document* document,
        std::string_view local_name,
        std::optional<std::string_view> element_namespace,
        std::optional<std::string_view> prefix,
        std::optional<std::string_view> is,
        bool) -> Element*
    {
        // Let result be null.
        Element* result = nullptr;

        // If registry is "default", then set registry to the result of looking up a custom element registry given document.
        // Let definition be the result of looking up a custom element definition given registry, namespace, localName, and is.
        void* definition = nullptr;

        // If definition is non-null, and definition’s name is not equal to its local name (i.e., definition represents a customized built-in element):
        auto definition_name = ""sv; // TODO(Peter): CustomRegistry
        if (definition != nullptr && definition_name != local_name)
        {
            // Let interface be the element interface for localName and the HTML namespace.
            // Set result to the result of creating an element internal given document, interface, localName, the HTML namespace, prefix, "undefined", is, and registry.
            // If synchronousCustomElements is true, then run this step while catching any exceptions:
            // Upgrade result using definition.
            // If this step threw an exception exception:
            // Report exception for definition’s constructor’s corresponding JavaScript object’s associated realm’s global object.
            // Set result’s custom element state to "failed".
            // Otherwise, enqueue a custom element upgrade reaction given result and definition.
        }
        // Otherwise, if definition is non-null:
        else if (definition != nullptr)
        {
            // If synchronousCustomElements is true:
            // Let C be definition’s constructor.
            // Set the surrounding agent’s active custom element constructor map[C] to registry.
            // Run these steps while catching any exceptions:
            // Set result to the result of constructing C, with no arguments.
            // Assert: result’s custom element state and custom element definition are initialized.
            // Assert: result’s namespace is the HTML namespace.
            // IDL enforces that result is an HTMLElement object, which all use the HTML namespace.
            // If result’s attribute list is not empty, then throw a "NotSupportedError" DOMException.
            // If result has children, then throw a "NotSupportedError" DOMException.
            // If result’s parent is not null, then throw a "NotSupportedError" DOMException.
            // If result’s node document is not document, then throw a "NotSupportedError" DOMException.
            // If result’s local name is not equal to localName, then throw a "NotSupportedError" DOMException.
            // Set result’s namespace prefix to prefix.
            // Set result’s is value to null.
            // Set result’s custom element registry to registry.
            // If any of these steps threw an exception exception:
            // Report exception for definition’s constructor’s corresponding JavaScript object’s associated realm’s global object.
            // Set result to the result of creating an element internal given document, HTMLUnknownElement, localName, the HTML namespace, prefix, "failed", null, and registry.
            // Remove the surrounding agent’s active custom element constructor map[C].
            // Under normal circumstances it will already have been removed at this point.
            // Otherwise:
            // Set result to the result of creating an element internal given document, HTMLElement, localName, the HTML namespace, prefix, "undefined", null, and registry.
            // Enqueue a custom element upgrade reaction given result and definition.
        }
        // Otherwise:
        else
        {
            // Let interface be the element interface for localName and namespace.
            auto interface = ElementInterface::Element;

            if (local_name == "html" && element_namespace == html_namespace)
            {
                // HTMLHtmlElement
                interface = ElementInterface::HTMLHtmlElement;
            }

            // Set result to the result of creating an element internal given document, interface, localName, namespace, prefix, "uncustomized", is, and registry.
            result = create_element_internal(document, interface, local_name, element_namespace, prefix, "uncustomized", is);

            // If namespace is the HTML namespace, and either localName is a valid custom element name or is is non-null, then set result’s custom element state to "undefined".
            bool local_name_valid_custom_element_name = false;
            if (element_namespace == html_namespace && (local_name_valid_custom_element_name || is.has_value()))
            {
                TODO();
                // result->custom_element_state = "undefined";
            }
        }

        // Return result.
        return result;
    }

    // https://dom.spec.whatwg.org/#create-an-element-internal
    auto TreeBuilder::create_element_internal(
        Document* document,
        ElementInterface interface,
        std::string_view local_name,
        std::optional<std::string_view> element_namespace,
        std::optional<std::string_view> prefix,
        std::string_view,
        std::optional<std::string_view>) -> Element*
    {
        // Let element be a new element that implements interface,
        // with namespace set to namespace,
        // namespace prefix set to prefix,
        // local name set to localName,
        // custom element registry set to registry,
        // custom element state set to state,
        // custom element definition set to null,
        // is value set to is,
        // and node document set to document.
        Element* element = nullptr;

        switch (interface)
        {
            case ElementInterface::Element:
            {
                element = new Element();
                break;
            }
            case ElementInterface::HTMLHtmlElement:
            {
                element = new HTMLHtmlElement();
                break;
            }
        }

        element->namespace_uri = element_namespace;
        element->namespace_prefix = prefix;
        element->local_name = local_name;
        // element->custom_element_registry = registry;
        // element->custom_element_state = state;
        // element->custom_element_definition = null;
        // element->is = is;
        element->m_document = document;

        // TODO(Peter):
        // Assert: element’s attribute list is empty.

        // Return element.
        return element;
    }


}
