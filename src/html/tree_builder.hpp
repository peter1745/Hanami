#pragma once

#include <memory>

#include "tokenizer.hpp"

#include <optional>
#include <string_view>

#include "tree_builder.hpp"

namespace hanami::html {

    using namespace std::literals;

    // https://infra.spec.whatwg.org/#namespaces
    static constexpr auto html_namespace = "http://www.w3.org/1999/xhtml"sv;
    static constexpr auto math_ml_namespace = "http://www.w3.org/1998/Math/MathML"sv;
    static constexpr auto svg_namespace = "http://www.w3.org/2000/svg"sv;
    static constexpr auto xlink_namespace = "http://www.w3.org/1999/xlink"sv;
    static constexpr auto xml_namespace = "http://www.w3.org/XML/1998/namespace"sv;
    static constexpr auto xmlns_namespace = "http://www.w3.org/2000/xmlns/"sv;

    enum class NodeType : uint8_t
    {
        Invalid = 0,
        Element = 1,
        Attribute = 2,
        Text = 3,
        CDATASection = 4,
        EntityReference = 5, // legacy
        Entity = 6, // legacy
        ProcessingInstruction = 7,
        Comment = 8,
        Document = 9,
        DocumentType = 10,
        DocumentFragment = 11,
        Notation = 12, // legacy
    };

    class Node;
    using NodeList = std::vector<Node*>;

    struct NodeListLocation
    {
        NodeList* list;
        NodeList::iterator iter;

        auto operator*() const -> Node*
        {
            if (iter < list->begin() || iter >= list->end())
            {
                return nullptr;
            }

            return *iter;
        }
    };

    class Document;

    // https://dom.spec.whatwg.org/#node
    class Node // : EventTarget
    {
    public:
        virtual ~Node() noexcept = default;

        [[nodiscard]]
        auto first_child() const noexcept -> Node*;

        [[nodiscard]]
        auto last_child() const noexcept -> Node*;

        auto insert_before(Node* node, Node* child) -> Node*;
        auto append_child(Node* node) -> Node*;

        [[nodiscard]]
        bool is_element() const noexcept { return m_type == NodeType::Element; }

        [[nodiscard]]
        auto children() const noexcept -> const NodeList& { return m_child_nodes; }

    protected:
        Node(NodeType type) noexcept
            : m_type(type)
        {
        }

    private:
        static void insert_node(Node* node, Node* parent, Node* child, std::optional<bool> suppress_observers = {});

    private:
        NodeType m_type = NodeType::Invalid;
        Document* m_document = nullptr;

        Node* m_parent = nullptr;
        NodeList m_child_nodes{};

        Node* m_previous_sibling = nullptr;
        Node* m_next_sibling = nullptr;

        friend class TreeBuilder;
    };

    // https://dom.spec.whatwg.org/#characterdata
    class CharacterData : public Node
    {
    protected:
        CharacterData(NodeType type, std::string_view data) noexcept
            : Node(type), m_data(data) {}
    private:
        std::string m_data;

        friend class TreeBuilder;
    };

    // https://dom.spec.whatwg.org/#interface-comment
    class Comment : public CharacterData
    {
    public:
        Comment(std::string_view data = "") noexcept
            : CharacterData(NodeType::Comment, data)
        {}
    };

    class DocumentType : public Node
    {
    public:
        DocumentType(std::string_view name, std::string_view public_id, std::string_view system_id) noexcept
            : Node(NodeType::DocumentType), m_name(name), m_public_id(public_id), m_system_id(system_id)
        {
        }

        [[nodiscard]]
        auto name() const noexcept -> std::string_view { return m_name; }

        [[nodiscard]]
        auto public_id() const noexcept -> std::string_view { return m_public_id; }

        [[nodiscard]]
        auto system_id() const noexcept -> std::string_view { return m_system_id; }

    private:
        std::string m_name;
        std::string m_public_id;
        std::string m_system_id;
    };

    // https://dom.spec.whatwg.org/#interface-element
    class Element : public Node
    {
    public:
        Element() noexcept
            : Node(NodeType::Element) {}

        // Null or a non-empty string.
        std::optional<std::string_view> namespace_uri{std::nullopt};

        // Null or a non-empty string.
        std::optional<std::string_view> namespace_prefix{std::nullopt};

        // A non-empty string.
        std::string local_name;

        // TODO(Peter): Custom Elements
        // custom element registry
        // Null or a CustomElementRegistry object.
        // custom element state
        // "undefined", "failed", "uncustomized", "precustomized", or "custom".
        // custom element definition
        // Null or a custom element definition.
        // is value
        // Null or a valid custom element name.

        [[nodiscard]]
        auto is_in_namespace(std::string_view value) const noexcept -> bool
        {
            return namespace_uri == value;
        }
    };

    // https://html.spec.whatwg.org/multipage/dom.html#htmlelement
    class HTMLElement : public Element
    {
    public:
        HTMLElement() noexcept
            : Element() {}
    };

    // https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
    class HTMLHtmlElement : public HTMLElement
    {
    public:
        HTMLHtmlElement() noexcept
            : HTMLElement() {}
    };

    // https://dom.spec.whatwg.org/#interface-text
    class Text : public CharacterData
    {
    public:
        Text(std::string_view data = "")
            : CharacterData(NodeType::Text, data) {}
    };

    // https://html.spec.whatwg.org/multipage/dom.html#document
    class Document : public Node
    {
    public:
        Document() noexcept
            : Node(NodeType::Document)
        {}

    private:
        Element* m_head = nullptr;
        bool m_scripting = false;

        friend class TreeBuilder;
    };

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
        void process_all_tokens(std::span<const Token> tokens);

    private:
        auto current_node() const noexcept -> Element*;
        auto adjusted_current_node() const noexcept -> Element*;

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
    };

}
