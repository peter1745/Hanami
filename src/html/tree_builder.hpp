#pragma once

#include "tokenizer.hpp"
#include "tree_builder.hpp"

#include "dom/document.hpp"
#include "dom/element.hpp"

namespace hanami::html {

    using namespace hanami::dom;

    // https://dom.spec.whatwg.org/#concept-element-interface
    enum class ElementInterface
    {
        Element,
        HTMLHtmlElement
    };

    enum class TreeInsertionMode
    {
        Initial,
        BeforeHTML,
        BeforeHead,
        InHead,
        InHeadNoScript,
        AfterHead,
        InBody,
        Text,
        InTable,
        InTableText,
        InCaption,
        InColumnGroup,
        InTableBody,
        InRow,
        InCell,
        InSelect,
        InSelectInTable,
        InTemplate,
        AfterBody,
        InFrameset,
        AfterFrameset,
        AfterAfterBody,
        AfterAfterFrameset,
    };

    class Tokenizer;

    class TreeBuilder
    {
    public:
        TreeBuilder(Tokenizer* tokenizer) noexcept;

        // https://html.spec.whatwg.org/multipage/parsing.html#tree-construction
        void process_token(const Token& token);
        void print_dom();

        auto document() const noexcept -> dom::Document*
        {
            return m_document.get();
        }

    private:

        // TODO(Peter): Doesn't belong here.
        void stop_parsing();

        auto current_node() const noexcept -> Element*;
        auto adjusted_current_node() const noexcept -> Element*;

        auto current_node_is_any_of(std::span<const std::string_view> tags) const noexcept -> bool;

        auto appropriate_insertion_place(std::optional<Node*> override_target = { std::nullopt }) const noexcept -> NodeListLocation;

        void insert_character(std::string_view data) noexcept;
        void insert_comment(std::string_view data, std::optional<NodeListLocation> pos = { std::nullopt }) noexcept;
        void insert_element_at_adjusted_insertion_location(Element* element, NodeListLocation adjusted_insertion_location);
        auto insert_html_element(const Token& token) -> Element*;
        auto insert_foreign_element(const Token& token, std::string_view element_namespace, bool only_add_to_element_stack) -> Element*;
        auto create_element_for_token(const Token& token, std::string_view element_namespace, Node* intended_parent) -> Element*;

        // TODO(Peter): "default", null, or a CustomElementRegistry object registry (default "default")
        auto create_element(
            Document* document,
            std::string_view local_name,
            std::optional<std::string_view> element_namespace,
            std::optional<std::string_view> prefix = { std::nullopt },
            std::optional<std::string_view> is = { std::nullopt },
            bool synchronous_custom_elements = false) -> Element*;

        // TODO(Peter): null or a CustomElementRegistry object registry
        auto create_element_internal(
            Document* document,
            ElementInterface interface,
            std::string_view local_name,
            std::optional<std::string_view> element_namespace,
            std::optional<std::string_view> prefix,
            std::string_view state,
            std::optional<std::string_view> is) -> Element*;

        void parse_generic_raw_text_element(const Token& token);
        void parse_generic_rcdata_element(const Token& token, bool generic_raw_text_parse = false);

    private:
        Tokenizer* m_tokenizer = nullptr;
        TreeInsertionMode m_original_insertion_mode = TreeInsertionMode::Initial;
        TreeInsertionMode m_insertion_mode = TreeInsertionMode::Initial;
        std::vector<Element*> m_open_elements{};
        std::unique_ptr<Document> m_document{};

        enum class FramesetOK { Ok, NotOk };
        FramesetOK m_frameset_ok = FramesetOK::Ok;
    };

}
