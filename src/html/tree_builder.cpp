#include "tree_builder.hpp"

#include "dom/text.hpp"
#include "dom/comment.hpp"
#include "dom/document.hpp"
#include "dom/html_element.hpp"
#include "dom/character_data.hpp"

#include <csignal>
#include <print>

#include "kori/core.hpp"

#define TODO(...) raise(SIGTRAP)
//#define TODO(...)

using namespace hanami::dom;

namespace hanami::html {

    TreeBuilder::TreeBuilder(Tokenizer* tokenizer) noexcept
        : m_tokenizer(tokenizer), m_document(std::make_unique<Document>())
    {
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#tree-construction
    void TreeBuilder::process_token(const Token& token)
    {
        auto is_special_character_token = [&] -> bool
        {
            // Character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
            auto* c = std::get_if<CharacterToken>(&token);
            return c && (c->data == '\t' || c->data == '\n' || c->data == '\f' || c->data == '\r' || c->data == ' ');
        };

        bool reprocess;

        do
        {
            reprocess = false;

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
                        reprocess = true;
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
                        reprocess = true;

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
                        reprocess = true;
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
                                parse_generic_rcdata_element(*t);
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
                                reprocess = true;
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
                        reprocess = true;
                        break;
                    }
                    case TreeInsertionMode::Text:
                    {
                        // A character token
                        if (const auto* c = std::get_if<CharacterToken>(&token); c)
                        {
                            // Insert the token's character.
                            insert_character({ &c->data, 1 });

                            // TODO(Peter):
                            // This can never be a U+0000 NULL character; the tokenizer converts those to U+FFFD REPLACEMENT CHARACTER characters.
                            break;
                        }

                        // An end-of-file token
                        if (const auto* eof = std::get_if<EOFToken>(&token); eof)
                        {
                            // Parse error.
                            // If the current node is a script element, then set its already started to true.
                            // Pop the current node off the stack of open elements.
                            m_open_elements.pop_back();

                            // Switch the insertion mode to the original insertion mode and reprocess the token.
                            m_insertion_mode = m_original_insertion_mode;
                            reprocess = true;
                            break;
                        }

                        // An end tag
                        if (const auto* t = std::get_if<EndTagToken>(&token); t)
                        {
                            // whose tag name is "script"
                            if (t->name == "script")
                            {
                                // If the active speculative HTML parser is null and the JavaScript execution context stack is empty, then perform a microtask checkpoint.
                                // Let script be the current node (which will be a script element).
                                // Pop the current node off the stack of open elements.
                                // Switch the insertion mode to the original insertion mode.
                                // Let the old insertion point have the same value as the current insertion point. Let the insertion point be just before the next input character.
                                // Increment the parser's script nesting level by one.
                                // If the active speculative HTML parser is null, then prepare the script element script. This might cause some script to execute, which might cause new characters to be inserted into the tokenizer, and might cause the tokenizer to output more tokens, resulting in a reentrant invocation of the parser.
                                // Decrement the parser's script nesting level by one. If the parser's script nesting level is zero, then set the parser pause flag to false.
                                // Let the insertion point have the value of the old insertion point. (In other words, restore the insertion point to its previous value. This value might be the "undefined" value.)
                                // At this stage, if the pending parsing-blocking script is not null, then:
                                // If the script nesting level is not zero:
                                // Set the parser pause flag to true, and abort the processing of any nested invocations of the tokenizer, yielding control back to the caller. (Tokenization will resume when the caller returns to the "outer" tree construction stage.)
                                // The tree construction stage of this particular parser is being called reentrantly, say from a call to document.write().
                                // Otherwise:
                                // While the pending parsing-blocking script is not null:
                                // Let the script be the pending parsing-blocking script.
                                // Set the pending parsing-blocking script to null.
                                // Start the speculative HTML parser for this instance of the HTML parser.
                                // Block the tokenizer for this instance of the HTML parser, such that the event loop will not run tasks that invoke the tokenizer.
                                // If the parser's Document has a style sheet that is blocking scripts or the script's ready to be parser-executed is false: spin the event loop until the parser's Document has no style sheet that is blocking scripts and the script's ready to be parser-executed becomes true.
                                // If this parser has been aborted in the meantime, return.
                                // This could happen if, e.g., while the spin the event loop algorithm is running, the Document gets destroyed, or the document.open() method gets invoked on the Document.
                                // Stop the speculative HTML parser for this instance of the HTML parser.
                                // Unblock the tokenizer for this instance of the HTML parser, such that tasks that invoke the tokenizer can again be run.
                                // Let the insertion point be just before the next input character.
                                // Increment the parser's script nesting level by one (it should be zero before this step, so this sets it to one).
                                // Execute the script element the script.
                                // Decrement the parser's script nesting level by one. If the parser's script nesting level is zero (which it always should be at this point), then set the parser pause flag to false.
                                // Let the insertion point be undefined again.
                                TODO();
                                break;
                            }

                            // Any other end tag
                            // Pop the current node off the stack of open elements.
                            m_open_elements.pop_back();

                            // Switch the insertion mode to the original insertion mode.
                            m_insertion_mode = m_original_insertion_mode;
                            break;
                        }

                        break;
                    }
                    case TreeInsertionMode::AfterHead:
                    {
                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (
                            token_is_character(token, '\t') ||
                            token_is_character(token, '\n') ||
                            token_is_character(token, '\f') ||
                            token_is_character(token, '\r') ||
                            token_is_character(token, ' '))
                        {
                            // Insert the character.
                            insert_character({ &std::get<CharacterToken>(token).data, 1 });
                            break;
                        }

                        // A comment token
                        if (token_is<CommentToken>(token))
                        {
                            // Insert a comment.
                            insert_comment(std::get<CommentToken>(token).data);
                            break;
                        }

                        // A DOCTYPE token
                        if (token_is<DOCTYPEToken>(token))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A start tag
                        if (const auto* start_tag = std::get_if<StartTagToken>(&token); start_tag)
                        {
                            // whose tag name is "html"
                            if (start_tag->name == "html")
                            {
                                // Process the token using the rules for the "in body" insertion mode.
                                TODO();
                                break;
                            }

                            // whose tag name is "body"
                            if (start_tag->name == "body")
                            {
                                // Insert an HTML element for the token.
                                insert_html_element(token);

                                // Set the frameset-ok flag to "not ok".
                                m_frameset_ok = FramesetOK::NotOk;

                                // Switch the insertion mode to "in body".
                                m_insertion_mode = TreeInsertionMode::InBody;
                                break;
                            }

                            // whose tag name is "frameset"
                            if (start_tag->name == "frameset")
                            {
                                // Insert an HTML element for the token.
                                // Switch the insertion mode to "in frameset".
                                TODO();
                                break;
                            }

                            // whose tag name is one of: "base", "basefont", "bgsound", "link", "meta", "noframes", "script", "style", "template", "title"
                            if (
                                start_tag->name == "base" || start_tag->name == "basefont" ||
                                start_tag->name == "bgsound" || start_tag->name == "link" ||
                                start_tag->name == "meta" || start_tag->name == "noframes" ||
                                start_tag->name == "script" || start_tag->name == "style" ||
                                start_tag->name == "template" || start_tag->name == "title"
                            )
                            {
                                // Parse error.
                                // Push the node pointed to by the head element pointer onto the stack of open elements.
                                // Process the token using the rules for the "in head" insertion mode.
                                // Remove the node pointed to by the head element pointer from the stack of open elements. (It might not be the current node at this point.)
                                // The head element pointer cannot be null at this point.
                                TODO();
                                break;
                            }
                        }

                        // An end tag
                        if (const auto* end_tag = std::get_if<EndTagToken>(&token); end_tag)
                        {
                            // whose tag name is "template"
                            // Process the token using the rules for the "in head" insertion mode.
                            // An end tag whose tag name is one of: "body", "html", "br"
                            // Act as described in the "anything else" entry below.
                            TODO();
                        }

                        // A start tag whose tag name is "head"
                        // Any other end tag
                        if (token_is_start_tag(token, "head") || std::get_if<EndTagToken>(&token))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // Anything else
                        //     Insert an HTML element for a "body" start tag token with no attributes.
                        //     Switch the insertion mode to "in body".
                        //     Reprocess the current token.
                        TODO();
                        break;
                    }
                    case TreeInsertionMode::InBody:
                    {
                        // A character token that is U+0000 NULL
                        if (token_is_character(token, '\0'))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (
                            token_is_character(token, '\t') ||
                            token_is_character(token, '\n') ||
                            token_is_character(token, '\f') ||
                            token_is_character(token, '\r') ||
                            token_is_character(token, ' '))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert the token's character.
                            insert_character({ &std::get<CharacterToken>(token).data, 1 });
                            break;
                        }

                        // Any other character token
                        if (const auto* c = std::get_if<CharacterToken>(&token); c)
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert the token's character.
                            insert_character({ &c->data, 1 });

                            // Set the frameset-ok flag to "not ok".
                            m_frameset_ok = FramesetOK::NotOk;
                            break;
                        }

                        // A comment token
                        if (token_is<CharacterToken>(token))
                        {
                            // Insert a comment.
                            TODO();
                            break;
                        }

                        // A DOCTYPE token
                        if (token_is<DOCTYPEToken>(token))
                        {
                            // Parse error. Ignore the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "html"
                        if (token_is_start_tag(token, "html"))
                        {
                            // Parse error.
                            // If there is a template element on the stack of open elements, then ignore the token.
                            // Otherwise, for each attribute on the token, check to see if the attribute is already present on the top element of the stack of open elements. If it is not, add the attribute and its corresponding value to that element.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "base", "basefont", "bgsound", "link", "meta", "noframes", "script", "style", "template", "title"
                        // An end tag whose tag name is "template"
                        if (
                            token_is_start_tag_any_of(token, { "base", "basefont", "bgsound", "link", "meta", "noframes", "script", "style", "template", "title" }) ||
                            token_is_end_tag(token, "template")
                        )
                        {
                            // Process the token using the rules for the "in head" insertion mode.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "body"
                        if (token_is_start_tag(token, "body"))
                        {
                            // Parse error.
                            // If the stack of open elements has only one node on it, if the second element on the stack of open elements is not a body element, or if there is a template element on the stack of open elements, then ignore the token. (fragment case or there is a template element on the stack)
                            // Otherwise, set the frameset-ok flag to "not ok"; then, for each attribute on the token, check to see if the attribute is already present on the body element (the second element) on the stack of open elements, and if it is not, add the attribute and its corresponding value to that element.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "frameset"
                        if (token_is_start_tag(token, "frameset"))
                        {
                            // Parse error.
                            // If the stack of open elements has only one node on it, or if the second element on the stack of open elements is not a body element, then ignore the token. (fragment case or there is a template element on the stack)
                            // If the frameset-ok flag is set to "not ok", ignore the token.
                            // Otherwise, run the following steps:
                            // Remove the second element on the stack of open elements from its parent node, if it has one.
                            // Pop all the nodes from the bottom of the stack of open elements, from the current node up to, but not including, the root html element.
                            // Insert an HTML element for the token.
                            // Switch the insertion mode to "in frameset".
                            TODO();
                            break;
                        }

                        // An end-of-file token
                        if (token_is<EOFToken>(token))
                        {
                            // If the stack of template insertion modes is not empty, then process the token using the rules for the "in template" insertion mode.
                            // Otherwise, follow these steps:
                            // If there is a node in the stack of open elements that is not either a dd element, a dt element, an li element, an optgroup element, an option element, a p element, an rb element, an rp element, an rt element, an rtc element, a tbody element, a td element, a tfoot element, a th element, a thead element, a tr element, the body element, or the html element, then this is a parse error.
                            // Stop parsing.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "body"
                        if (token_is_end_tag(token, "body"))
                        {
                            // TODO(Peter):
                            // If the stack of open elements does not have a body element in scope, this is a parse error; ignore the token.
                            // Otherwise, if there is a node in the stack of open elements that is not either a dd element, a dt element, an li element, an optgroup element, an option element, a p element, an rb element, an rp element, an rt element, an rtc element, a tbody element, a td element, a tfoot element, a th element, a thead element, a tr element, the body element, or the html element, then this is a parse error.

                            // Switch the insertion mode to "after body".
                            m_insertion_mode = TreeInsertionMode::AfterBody;
                            break;
                        }

                        // An end tag whose tag name is "html"
                        if (token_is_end_tag(token, "html"))
                        {
                            // If the stack of open elements does not have a body element in scope, this is a parse error; ignore the token.
                            // Otherwise, if there is a node in the stack of open elements that is not either a dd element, a dt element, an li element, an optgroup element, an option element, a p element, an rb element, an rp element, an rt element, an rtc element, a tbody element, a td element, a tfoot element, a th element, a thead element, a tr element, the body element, or the html element, then this is a parse error.
                            // Switch the insertion mode to "after body".
                            // Reprocess the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "address", "article", "aside", "blockquote", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "main", "menu", "nav", "ol", "p", "search", "section", "summary", "ul"
                        if (token_is_start_tag_any_of(token, { "address", "article", "aside", "blockquote", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "main", "menu", "nav", "ol", "p", "search", "section", "summary", "ul" }))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            for (auto* element : m_open_elements)
                            {
                                if (element->local_name == "p")
                                {
                                    TODO();
                                }
                            }

                            // Insert an HTML element for the token.
                            insert_html_element(token);
                            break;
                        }

                        // A start tag whose tag name is one of: "h1", "h2", "h3", "h4", "h5", "h6"
                        if (token_is_start_tag_any_of(token, { "h1", "h2", "h3", "h4", "h5", "h6" }))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // If the current node is an HTML element whose tag name is one of "h1", "h2", "h3", "h4", "h5", or "h6", then this is a parse error; pop the current node off the stack of open elements.
                            // Insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "pre", "listing"
                        if (token_is_start_tag_any_of(token, { "pre", "listing" }))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // Insert an HTML element for the token.
                            // If the next token is a U+000A LINE FEED (LF) character token, then ignore that token and move on to the next one. (Newlines at the start of pre blocks are ignored as an authoring convenience.)
                            // Set the frameset-ok flag to "not ok".
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "form"
                        if (token_is_start_tag(token, "form"))
                        {
                            // If the form element pointer is not null, and there is no template element on the stack of open elements, then this is a parse error; ignore the token.
                            // Otherwise:
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // Insert an HTML element for the token, and, if there is no template element on the stack of open elements, set the form element pointer to point to the element created.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "li"
                        if (token_is_start_tag(token, "li"))
                        {
                            // Run these steps:
                            // Set the frameset-ok flag to "not ok".
                            // Initialize node to be the current node (the bottommost node of the stack).
                            // Loop: If node is an li element, then run these substeps:
                            // Generate implied end tags, except for li elements.
                            // If the current node is not an li element, then this is a parse error.
                            // Pop elements from the stack of open elements until an li element has been popped from the stack.
                            // Jump to the step labeled done below.
                            // If node is in the special category, but is not an address, div, or p element, then jump to the step labeled done below.
                            // Otherwise, set node to the previous entry in the stack of open elements and return to the step labeled loop.
                            // Done: If the stack of open elements has a p element in button scope, then close a p element.
                            // Finally, insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "dd", "dt"
                        if (token_is_start_tag_any_of(token, { "dd", "dt" }))
                        {
                            // Run these steps:
                            // Set the frameset-ok flag to "not ok".
                            // Initialize node to be the current node (the bottommost node of the stack).
                            // Loop: If node is a dd element, then run these substeps:
                            // Generate implied end tags, except for dd elements.
                            // If the current node is not a dd element, then this is a parse error.
                            // Pop elements from the stack of open elements until a dd element has been popped from the stack.
                            // Jump to the step labeled done below.
                            // If node is a dt element, then run these substeps:
                            // Generate implied end tags, except for dt elements.
                            // If the current node is not a dt element, then this is a parse error.
                            // Pop elements from the stack of open elements until a dt element has been popped from the stack.
                            // Jump to the step labeled done below.
                            // If node is in the special category, but is not an address, div, or p element, then jump to the step labeled done below.
                            // Otherwise, set node to the previous entry in the stack of open elements and return to the step labeled loop.
                            // Done: If the stack of open elements has a p element in button scope, then close a p element.
                            // Finally, insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "plaintext"
                        if (token_is_start_tag(token, "plaintext"))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // Insert an HTML element for the token.
                            // Switch the tokenizer to the PLAINTEXT state.
                            // Once a start tag with the tag name "plaintext" has been seen, all remaining tokens will be character tokens (and a final end-of-file token) because there is no way to switch the tokenizer out of the PLAINTEXT state. However, as the tree builder remains in its existing insertion mode, it might reconstruct the active formatting elements while processing those character tokens. This means that the parser can insert other elements into the plaintext element.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "button"
                        if (token_is_start_tag(token, "button"))
                        {
                            // If the stack of open elements has a button element in scope, then run these substeps:
                            // Parse error.
                            // Generate implied end tags.
                            // Pop elements from the stack of open elements until a button element has been popped from the stack.
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            // Set the frameset-ok flag to "not ok".
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is one of: "address", "article", "aside", "blockquote", "button", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "listing", "main", "menu", "nav", "ol", "pre", "search", "section", "summary", "ul"
                        if (token_is_end_tag_any_of(token, { "address", "article", "aside", "blockquote", "button", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "listing", "main", "menu", "nav", "ol", "pre", "search", "section", "summary", "ul" }))
                        {
                            // If the stack of open elements does not have an element in scope that is an HTML element with the same tag name as that of the token, then this is a parse error; ignore the token.
                            // Otherwise, run these steps:
                            // Generate implied end tags.
                            while (current_node_is_any_of({ "dd", "dt", "li", "optgroup", "option", "p", "rb", "rp", "rt", "rtc" }))
                            {
                                m_open_elements.pop_back();
                            }

                            // If the current node is not an HTML element with the same tag name as that of the token, then this is a parse error.
                            if (dynamic_cast<HTMLElement*>(current_node()) && current_node()->local_name == token_tag_name(token))
                            {
                                TODO();
                                break;
                            }

                            // Pop elements from the stack of open elements until an HTML element with the same tag name as the token has been popped from the stack.
                            while (auto* current = current_node())
                            {
                                m_open_elements.pop_back();

                                if (current->local_name == token_tag_name(token))
                                {
                                    break;
                                }
                            }

                            break;
                        }

                        // An end tag whose tag name is "form"
                        if (token_is_end_tag(token, "form"))
                        {
                            // If there is no template element on the stack of open elements, then run these substeps:
                            // Let node be the element that the form element pointer is set to, or null if it is not set to an element.
                            // Set the form element pointer to null.
                            // If node is null or if the stack of open elements does not have node in scope, then this is a parse error; return and ignore the token.
                            // Generate implied end tags.
                            // If the current node is not node, then this is a parse error.
                            // Remove node from the stack of open elements.
                            // If there is a template element on the stack of open elements, then run these substeps instead:
                            // If the stack of open elements does not have a form element in scope, then this is a parse error; return and ignore the token.
                            // Generate implied end tags.
                            // If the current node is not a form element, then this is a parse error.
                            // Pop elements from the stack of open elements until a form element has been popped from the stack.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "p"
                        if (token_is_end_tag(token, "p"))
                        {
                            // If the stack of open elements does not have a p element in button scope, then this is a parse error; insert an HTML element for a "p" start tag token with no attributes.
                            // Close a p element.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "li"
                        if (token_is_end_tag(token, "li"))
                        {
                            // If the stack of open elements does not have an li element in list item scope, then this is a parse error; ignore the token.
                            // Otherwise, run these steps:
                            // Generate implied end tags, except for li elements.
                            // If the current node is not an li element, then this is a parse error.
                            // Pop elements from the stack of open elements until an li element has been popped from the stack.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is one of: "dd", "dt"
                        if (token_is_end_tag_any_of(token, { "dd", "dt" }))
                        {
                            // If the stack of open elements does not have an element in scope that is an HTML element with the same tag name as that of the token, then this is a parse error; ignore the token.
                            // Otherwise, run these steps:
                            // Generate implied end tags, except for HTML elements with the same tag name as the token.
                            // If the current node is not an HTML element with the same tag name as that of the token, then this is a parse error.
                            // Pop elements from the stack of open elements until an HTML element with the same tag name as the token has been popped from the stack.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is one of: "h1", "h2", "h3", "h4", "h5", "h6"
                        if (token_is_end_tag_any_of(token, { "h1", "h2", "h3", "h4", "h5", "h6" }))
                        {
                            // If the stack of open elements does not have an element in scope that is an HTML element and whose tag name is one of "h1", "h2", "h3", "h4", "h5", or "h6", then this is a parse error; ignore the token.
                            // Otherwise, run these steps:
                            // Generate implied end tags.
                            // If the current node is not an HTML element with the same tag name as that of the token, then this is a parse error.
                            // Pop elements from the stack of open elements until an HTML element whose tag name is one of "h1", "h2", "h3", "h4", "h5", or "h6" has been popped from the stack.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "sarcasm"
                        if (token_is_end_tag(token, "sarcasm"))
                        {
                            // Take a deep breath, then act as described in the "any other end tag" entry below.
                        }

                        // A start tag whose tag name is "a"
                        if (token_is_start_tag(token, "a"))
                        {
                            // If the list of active formatting elements contains an a element between the end of the list and the last marker on the list (or the start of the list if there is no marker on the list), then this is a parse error; run the adoption agency algorithm for the token, then remove that element from the list of active formatting elements and the stack of open elements if the adoption agency algorithm didn't already remove it (it might not have if the element is not in table scope).
                            // In the non-conforming stream <a href="a">a<table><a href="b">b</table>x, the first a element would be closed upon seeing the second one, and the "x" character would be inside a link to "b", not to "a". This is despite the fact that the outer a element is not in table scope (meaning that a regular </a> end tag at the start of the table wouldn't close the outer a element). The result is that the two a elements are indirectly nested inside each other — non-conforming markup will often result in non-conforming DOMs when parsed.
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token. Push onto the list of active formatting elements that element.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "b", "big", "code", "em", "font", "i", "s", "small", "strike", "strong", "tt", "u"
                        if (token_is_start_tag_any_of(token, { "b", "big", "code", "em", "font", "i", "s", "small", "strike", "strong", "tt", "u" }))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token. Push onto the list of active formatting elements that element.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "nobr"
                        if (token_is_start_tag(token, "nobr"))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // If the stack of open elements has a nobr element in scope, then this is a parse error; run the adoption agency algorithm for the token, then once again reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token. Push onto the list of active formatting elements that element.
                            // An end tag whose tag name is one of: "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small", "strike", "strong", "tt", "u"
                            // Run the adoption agency algorithm for the token.
                            // A start tag whose tag name is one of: "applet", "marquee", "object"
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            // Insert a marker at the end of the list of active formatting elements.
                            // Set the frameset-ok flag to "not ok".
                            TODO();
                            break;
                        }

                        // An end tag token whose tag name is one of: "applet", "marquee", "object"
                        if (token_is_end_tag_any_of(token, { "applet", "marquee", "object" }))
                        {
                            // If the stack of open elements does not have an element in scope that is an HTML element with the same tag name as that of the token, then this is a parse error; ignore the token.
                            // Otherwise, run these steps:
                            // Generate implied end tags.
                            // If the current node is not an HTML element with the same tag name as that of the token, then this is a parse error.
                            // Pop elements from the stack of open elements until an HTML element with the same tag name as the token has been popped from the stack.
                            // Clear the list of active formatting elements up to the last marker.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "table"
                        if (token_is_start_tag(token, "table"))
                        {
                            // If the Document is not set to quirks mode, and the stack of open elements has a p element in button scope, then close a p element.
                            // Insert an HTML element for the token.
                            // Set the frameset-ok flag to "not ok".
                            // Switch the insertion mode to "in table".
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "br"
                        if (token_is_end_tag(token, "br"))
                        {
                            // Parse error. Drop the attributes from the token, and act as described in the next entry; i.e. act as if this was a "br" start tag token with no attributes, rather than the end tag token that it actually is.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "area", "br", "embed", "img", "keygen", "wbr"
                        if (token_is_start_tag_any_of(token, { "area", "br", "embed", "img", "keygen", "wbr" }))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            insert_html_element(token);

                            // Immediately pop the current node off the stack of open elements.
                            m_open_elements.pop_back();

                            // Acknowledge the token's self-closing flag, if it is set.
                            // Set the frameset-ok flag to "not ok".

                            m_frameset_ok = FramesetOK::NotOk;
                            break;
                        }

                        // A start tag whose tag name is "input"
                        if (token_is_start_tag(token, "input"))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            insert_html_element(token);

                            // Immediately pop the current node off the stack of open elements.
                            m_open_elements.pop_back();

                            // Acknowledge the token's self-closing flag, if it is set.
                            // If the token does not have an attribute with the name "type", or if it does, but that attribute's value is not an ASCII case-insensitive match for the string "hidden"
                            auto type = get_token_attribute_value(&std::get<StartTagToken>(token), "type").value_or("");
                            if (equals_case_insensitive(type, "hidden"))
                            {
                                // then: set the frameset-ok flag to "not ok".
                                m_frameset_ok = FramesetOK::NotOk;
                            }

                            break;
                        }

                        // A start tag whose tag name is one of: "param", "source", "track"
                        if (token_is_start_tag_any_of(token, { "param", "source", "track" }))
                        {
                            // Insert an HTML element for the token. Immediately pop the current node off the stack of open elements.
                            // Acknowledge the token's self-closing flag, if it is set.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "hr"
                        if (token_is_start_tag(token, "hr"))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // Insert an HTML element for the token. Immediately pop the current node off the stack of open elements.
                            // Acknowledge the token's self-closing flag, if it is set.
                            // Set the frameset-ok flag to "not ok".
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "image"
                        if (token_is_start_tag(token, "image"))
                        {
                            // Parse error. Change the token's tag name to "img" and reprocess it. (Don't ask.)
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "textarea"
                        if (token_is_start_tag(token, "textarea"))
                        {
                            // Run these steps:
                            // Insert an HTML element for the token.
                            // If the next token is a U+000A LINE FEED (LF) character token, then ignore that token and move on to the next one. (Newlines at the start of textarea elements are ignored as an authoring convenience.)
                            // Switch the tokenizer to the RCDATA state.
                            // Set the original insertion mode to the current insertion mode.
                            // Set the frameset-ok flag to "not ok".
                            // Switch the insertion mode to "text".
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "xmp"
                        if (token_is_start_tag(token, "xmp"))
                        {
                            // If the stack of open elements has a p element in button scope, then close a p element.
                            // Reconstruct the active formatting elements, if any.
                            // Set the frameset-ok flag to "not ok".
                            // Follow the generic raw text element parsing algorithm.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "iframe"
                        if (token_is_start_tag(token, "iframe"))
                        {
                            // Set the frameset-ok flag to "not ok".
                            // Follow the generic raw text element parsing algorithm.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "noembed"
                        // A start tag whose tag name is "noscript", if the scripting flag is enabled
                        if (
                            token_is_start_tag(token, "noembed") ||
                            (token_is_start_tag(token, "noscript") && m_document->m_scripting)
                        )
                        {
                            // Follow the generic raw text element parsing algorithm.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "select"
                        if (token_is_start_tag(token, "select"))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            // Set the frameset-ok flag to "not ok".
                            // If the insertion mode is one of "in table", "in caption", "in table body", "in row", or "in cell", then switch the insertion mode to "in select in table". Otherwise, switch the insertion mode to "in select".
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "optgroup", "option"
                        if (token_is_start_tag_any_of(token, { "optgroup", "option" }))
                        {
                            // If the current node is an option element, then pop the current node off the stack of open elements.
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "rb", "rtc"
                        if (token_is_start_tag_any_of(token, { "rb", "rtc" }))
                        {
                            // If the stack of open elements has a ruby element in scope, then generate implied end tags. If the current node is not now a ruby element, this is a parse error.
                            // Insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "rp", "rt"
                        if (token_is_start_tag_any_of(token, { "rp", "rt" }))
                        {
                            // If the stack of open elements has a ruby element in scope, then generate implied end tags, except for rtc elements. If the current node is not now a rtc element or a ruby element, this is a parse error.
                            // Insert an HTML element for the token.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "math"
                        if (token_is_start_tag(token, "math"))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Adjust MathML attributes for the token. (This fixes the case of MathML attributes that are not all lowercase.)
                            // Adjust foreign attributes for the token. (This fixes the use of namespaced attributes, in particular XLink.)
                            // Insert a foreign element for the token, with MathML namespace and false.
                            // If the token has its self-closing flag set, pop the current node off the stack of open elements and acknowledge the token's self-closing flag.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is "svg"
                        if (token_is_start_tag(token, "svg"))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Adjust SVG attributes for the token. (This fixes the case of SVG attributes that are not all lowercase.)
                            // Adjust foreign attributes for the token. (This fixes the use of namespaced attributes, in particular XLink in SVG.)
                            // Insert a foreign element for the token, with SVG namespace and false.
                            // If the token has its self-closing flag set, pop the current node off the stack of open elements and acknowledge the token's self-closing flag.
                            TODO();
                            break;
                        }

                        // A start tag whose tag name is one of: "caption", "col", "colgroup", "frame", "head", "tbody", "td", "tfoot", "th", "thead", "tr"
                        if (token_is_start_tag_any_of(token, { "caption", "col", "colgroup", "frame", "head", "tbody", "td", "tfoot", "th", "thead", "tr" }))
                        {
                            // Parse error. Ignore the token.
                            TODO();
                            break;
                        }

                        // Any other start tag
                        if (token_is<StartTagToken>(token))
                        {
                            // Reconstruct the active formatting elements, if any.
                            // Insert an HTML element for the token.
                            // This element will be an ordinary element. With one exception: if the scripting flag is disabled, it can also be a noscript element.
                            TODO();
                            break;
                        }

                        // Any other end tag
                        if (token_is<StartTagToken>(token))
                        {
                            // Run these steps:
                            // Initialize node to be the current node (the bottommost node of the stack).
                            // Loop: If node is an HTML element with the same tag name as the token, then:
                            // Generate implied end tags, except for HTML elements with the same tag name as the token.
                            // If node is not the current node, then this is a parse error.
                            // Pop all the nodes from the current node up to node, including node, then stop these steps.
                            // Otherwise, if node is in the special category, then this is a parse error; ignore the token, and return.
                            // Set node to the previous entry in the stack of open elements.
                            // Return to the step labeled loop.
                            TODO();
                            break;
                        }

                        break;
                    }
                    case TreeInsertionMode::AfterBody:
                    {
                        // A character token that is one of U+0009 CHARACTER TABULATION, U+000A LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or U+0020 SPACE
                        if (token_is_character(token, '\t') || token_is_character(token, '\n') || token_is_character(token, '\f') || token_is_character(token, '\r') || token_is_character(token, ' '))
                        {
                            // Process the token using the rules for the "in body" insertion mode.
                            insert_character({ &std::get<CharacterToken>(token).data, 1 });
                            break;
                        }

                        // A comment token
                        if (token_is<CommentToken>(token))
                        {
                            // Insert a comment as the last child of the first element in the stack of open elements (the html element).
                            TODO();
                            break;
                        }

                        // A DOCTYPE token
                        if (token_is<DOCTYPEToken>(token))
                        {
                            // Parse error. Ignore the token.
                            break;
                        }

                        // A start tag whose tag name is "html"
                        if (token_is_start_tag(token, "html"))
                        {
                            // Process the token using the rules for the "in body" insertion mode.
                            TODO();
                            break;
                        }

                        // An end tag whose tag name is "html"
                        if (token_is_end_tag(token, "html"))
                        {
                            // If the parser was created as part of the HTML fragment parsing algorithm, this is a parse error; ignore the token. (fragment case)
                            // Otherwise, switch the insertion mode to "after after body".
                            TODO();
                            break;
                        }

                        // An end-of-file token
                        if (token_is<EOFToken>(token))
                        {
                            // Stop parsing.
                            stop_parsing();
                            break;
                        }

                        // Anything else
                        //     Parse error. Switch the insertion mode to "in body" and reprocess the token.
                        m_insertion_mode = TreeInsertionMode::InBody;
                        reprocess = true;
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
        } while (reprocess);
    }

    void TreeBuilder::print_dom()
    {
        static int32_t num_indents = -1;
        static bool exclude_empty_cdata = false;

        auto print_node = [&](this auto&& self, Node* node) -> void
        {
            ++num_indents;
            kori_defer { --num_indents; };

            auto indents = std::string{};
            for (int32_t i = 0; i < num_indents; i++)
                indents += '\t';

            if (const auto* cdata = dynamic_cast<CharacterData*>(node))
            {
                if ((cdata->m_data == " " || cdata->m_data == "\n" || cdata->m_data == "\t" || cdata->m_data == "\f") && exclude_empty_cdata && node->m_child_nodes.empty())
                {
                    return;
                }
            }

            std::println("{}- {}:", indents, node_type_str(node->m_type));

            indents += '\t';

            if (const auto* elem = dynamic_cast<Element*>(node))
            {
                std::println("{}Namespace URI: {}", indents, elem->namespace_uri.value_or(""));
                std::println("{}Namespace Prefix: {}", indents, elem->namespace_prefix.value_or(""));
                std::println("{}Local Name: {}", indents, elem->local_name);
            }

            if (const auto* cdata = dynamic_cast<CharacterData*>(node))
            {
                std::println("{}Data: {}", indents, cdata->m_data);
            }

            if (!node->m_child_nodes.empty())
            {
                std::println("{}Children:", indents);
                for (auto* child : node->m_child_nodes)
                {
                    self(child);
                }
            }
        };

        print_node(m_document.get());
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#stop-parsing
    void TreeBuilder::stop_parsing()
    {
        // If the active speculative HTML parser is not null, then stop the speculative HTML parser and return.
        // Set the insertion point to undefined.
        // Update the current document readiness to "interactive".
        // Pop all the nodes off the stack of open elements.
        m_open_elements.clear();

        // While the list of scripts that will execute when the document has finished parsing is not empty:
        // Spin the event loop until the first script in the list of scripts that will execute when the document has finished parsing has its ready to be parser-executed set to true and the parser's Document has no style sheet that is blocking scripts.
        // Execute the script element given by the first script in the list of scripts that will execute when the document has finished parsing.
        // Remove the first script element from the list of scripts that will execute when the document has finished parsing (i.e. shift out the first entry in the list).
        // Queue a global task on the DOM manipulation task source given the Document's relevant global object to run the following substeps:
        // Set the Document's load timing info's DOM content loaded event start time to the current high resolution time given the Document's relevant global object.
        // Fire an event named DOMContentLoaded at the Document object, with its bubbles attribute initialized to true.
        // Set the Document's load timing info's DOM content loaded event end time to the current high resolution time given the Document's relevant global object.
        // Enable the client message queue of the ServiceWorkerContainer object whose associated service worker client is the Document object's relevant settings object.
        // Invoke WebDriver BiDi DOM content loaded with the Document's browsing context, and a new WebDriver BiDi navigation status whose id is the Document object's during-loading navigation ID for WebDriver BiDi, status is "pending", and url is the Document object's URL.
        // Spin the event loop until the set of scripts that will execute as soon as possible and the list of scripts that will execute in order as soon as possible are empty.
        // Spin the event loop until there is nothing that delays the load event in the Document.
        // Queue a global task on the DOM manipulation task source given the Document's relevant global object to run the following steps:
        // Update the current document readiness to "complete".
        // If the Document object's browsing context is null, then abort these steps.
        // Let window be the Document's relevant global object.
        // Set the Document's load timing info's load event start time to the current high resolution time given window.
        // Fire an event named load at window, with legacy target override flag set.
        // Invoke WebDriver BiDi load complete with the Document's browsing context, and a new WebDriver BiDi navigation status whose id is the Document object's during-loading navigation ID for WebDriver BiDi, status is "complete", and url is the Document object's URL.
        // Set the Document object's during-loading navigation ID for WebDriver BiDi to null.
        // Set the Document's load timing info's load event end time to the current high resolution time given window.
        // Assert: Document's page showing is false.
        // Set the Document's page showing to true.
        // Fire a page transition event named pageshow at window with false.
        // Completely finish loading the Document.
        // Queue the navigation timing entry for the Document.
        // If the Document's print when loaded flag is set, then run the printing steps.
        // The Document is now ready for post-load tasks.
        // When the user agent is to abort a parser, it must run the following steps:
        // Throw away any pending content in the input stream, and discard any future content that would have been added to it.
        // Stop the speculative HTML parser for this HTML parser.
        // Update the current document readiness to "interactive".
        // Pop all the nodes off the stack of open elements.
        // Update the current document readiness to "complete".

        print_dom();
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

    auto TreeBuilder::current_node_is_any_of(std::span<const std::string_view> tags) const noexcept -> bool
    {
        return std::ranges::any_of(tags, [&](auto tag)
        {
            return current_node()->local_name == tag;
        });
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
        if (auto* t = dynamic_cast<Text*>(*(adjusted_insertion_location--)); t)
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

    // https://html.spec.whatwg.org/multipage/parsing.html#generic-rcdata-element-parsing-algorithm
    void TreeBuilder::parse_generic_raw_text_element(const Token& token)
    {
        parse_generic_rcdata_element(token, true);
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#generic-rcdata-element-parsing-algorithm
    void TreeBuilder::parse_generic_rcdata_element(const Token& token, bool generic_raw_text_parse)
    {
        // Insert an HTML element for the token.
        insert_html_element(token);

        // If the algorithm that was invoked is the generic raw text element parsing algorithm
        if (generic_raw_text_parse)
        {
            // switch the tokenizer to the RAWTEXT state;
            m_tokenizer->set_state(Tokenizer::State::RAWTEXT);
        }
        else
        {
            // otherwise the algorithm invoked was the generic RCDATA element parsing algorithm,
            // switch the tokenizer to the RCDATA state.
            m_tokenizer->set_state(Tokenizer::State::RCDATA);
        }

        // Set the original insertion mode to the current insertion mode.
        m_original_insertion_mode = m_insertion_mode;

        // Then, switch the insertion mode to "text".
        m_insertion_mode = TreeInsertionMode::Text;
    }

}
