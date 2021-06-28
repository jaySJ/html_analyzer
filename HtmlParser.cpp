
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <sstream>

#define HTML_PARSER_DEBUG
#undef HTML_PARSER_DEBUG

// #region obsolete_AST_based_version
namespace client
{
    namespace fusion = boost::fusion;
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    struct mini_html;

    typedef boost::variant<boost::recursive_wrapper<mini_html>, std::string> mini_html_node;

    struct mini_html
    {
        std::string tag_name;
        std::vector<mini_html_node> children;
    };
}

// We need to tell fusion about our mini_html struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    client::mini_html,
    (std::string, tag_name)
    (std::vector<client::mini_html_node>, children)
)

namespace client
{
    const int tabsize = 4;

    // TODO: Need a thread-safe approach iif it is to work
    // in a multi-threaded manner - requires more work
    static int gNumLeafNodes = 0;
    static int gNumNodes = 0;
    static int gDivNodeCount = 0;

    void tab(int indent)
    {
#ifdef HTML_PARSER_DEBUG
        for (int i = 0; i < indent; ++i)
            std::cout << ' ';
#endif
    }
    void incrLeafCount()
    {
        gNumLeafNodes++;
    }
    void incrNodeCount()
    {
        gNumNodes++;
    }
    void incrDivCount()
    {
        gDivNodeCount ++;
    }
    struct mini_html_printer
    {
        mini_html_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(mini_html const& html) const;

        int indent;
    };

    struct mini_html_node_printer : boost::static_visitor<>
    {
        mini_html_node_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(mini_html const& html) const
        {
            mini_html_printer(indent+tabsize)(html);
        }

        void operator()(std::string const& text) const
        {
            tab(indent+tabsize);
#ifdef HTML_PARSER_DEBUG
            std::cout << "text: \"" << text << '"' << std::endl;
#endif
        }

        int indent;
    };

    void mini_html_printer::operator()(mini_html const& html) const
    {
        tab(indent);
#ifdef HTML_PARSER_DEBUG
        std::cout << "tag: " << html.tag_name ;
#endif
         if (html.tag_name == "div") {
             // Found div increment div count
            incrDivCount();
         }
         if (html.children.size() == 0) {
            // No children - increment leaf count
            incrLeafCount();
         }
         incrNodeCount();
#ifdef HTML_PARSER_DEBUG
        std::cout << std::endl;
#endif
        tab(indent);
#ifdef HTML_PARSER_DEBUG
        std::cout << '{' << std::endl;
#endif
        BOOST_FOREACH(mini_html_node const& node, html.children)
        {
            boost::apply_visitor(mini_html_node_printer(indent), node);
        }

        tab(indent);

#ifdef HTML_PARSER_DEBUG
        std::cout << '}' << std::endl;
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    // Html grammar definition
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct mini_html_grammar
      : qi::grammar<Iterator, mini_html(), qi::locals<std::string>, ascii::space_type>
    {
        mini_html_grammar()
          : mini_html_grammar::base_type(html)
        {
            using namespace boost::spirit::ascii;
            using qi::lit;
            using qi::lexeme;
            using qi::eoi;
            using qi::eol;
            using qi::attr;
            using qi::omit;
            using qi::alnum;
            using qi::as_string;
            using ascii::char_;
            using ascii::string;
            using ascii::space;
            using namespace qi::labels;

            text %= lexeme[+(char_ - '<')];
            
            node %= html | text;

            start_tag %=
                    '<'
                >>  !(lit('/'))
                >>  lexeme[+(char_ - '>')] 
                >>  '>'
            ;

            end_tag =
                    "</"
                >>  string(_r1)
                >>  '>'
            ;
            
            html %=
                    start_tag[_a = _1]
                >>  *node 
                >>  end_tag(_a)
            ;
        }

        qi::rule<Iterator, mini_html(), qi::locals<std::string>, ascii::space_type> html;
        qi::rule<Iterator, mini_html_node(), ascii::space_type> node;
        qi::rule<Iterator, std::string(), ascii::space_type> text;
        qi::rule<Iterator, std::string(), ascii::space_type> start_tag;
        qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
    };
    //]
}

// #endregion obsolete_AST_based_version


std::tuple<uint64_t, uint64_t, uint64_t, std::string> getCleanDomTree(const std::string & rawHtml) {
    std::regex line_endings("(.*)(\n)*(.*)");
    auto it = std::sregex_iterator(rawHtml.begin(), rawHtml.end(), line_endings);

    // Cleanup line-endings
    // Given a raw html, transforms it to have no line endings
    std::stringstream firstStage;
    for (std::sregex_iterator i = it; i != std::sregex_iterator(); ++i) {
        std::smatch match = *i;
        firstStage << match[1] << match[3];
    }
#ifdef HTML_PARSER_DEBUG
    std::cout << std::endl << " Stage 1 Output" << std::endl << firstStage.str() <<std::endl;
#endif
    // Remove angle brackets from some simple expressions in <scripts>
    // TODO: This kind of pre-filtering of expressions using regex would be prone to 
    // parsing errors due to lack of context.
    std::regex remove_js_expr("([=]*(<|>)[0-9=+])");
    auto firstStageOut = firstStage.str();
    firstStage.clear();
    std::stringstream secondStage;
    secondStage << std::regex_replace(firstStageOut, remove_js_expr,"");
#ifdef HTML_PARSER_DEBUG
    std::cout << std::endl << " Stage 2 Output" << std::endl << secondStage.str() <<std::endl;
#endif
    // Given html with no line-endings transforms to only include HTML tags
    // with beginning and endings. For some meaningless ones like doctype
    // or comments, retuns as empty.
    // Examples:    
    //      <>	    ::FROM::	<!DOCTYPE>
    //      <html>	::FROM::	<html>
    //      <meta>	::FROM::	<meta http-equiv="Content-Type" content="text/html; charset=US-ASCII">
    //      <meta/>	::FROM::	<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
    //      </head>	::FROM::	</head>
    std::regex tag_capture("(<([a-zA-Z0-9/]*)(.*?)(/*)>)");
    auto secondStateOut = secondStage.str();
    secondStage.clear();
    auto it2 = std::sregex_iterator(secondStateOut.begin(), secondStateOut.end(), tag_capture);
    
    const std::vector<std::string> selfClosingTags = {
        "area",
        "base",
        "br",
        "col",
        "embed",
        "hr",
        "img",
        "svg",
        "input",
        "link",
        "meta",
        "param",
        "source",
        "track",
        "wbr",
        "command",
        "keygen",
        "menuitem"
    };

    // As we are processing the input, we can count the number of elements
    // as well as those that have a div tag.
    uint64_t numDivNodes = 0;
    uint64_t numNodes = 0;
    uint64_t numLeafNodes = 0;

    std::stringstream thirdStage;
    for (std::sregex_iterator i = it2; i != std::sregex_iterator(); ++i) {
        std::smatch match = *i;
        std::stringstream tag;
        
        // Special handling for <link> and <img> elements
        // These are not expected to have corresponding end tags i.e. /> or </ 
        // So we can either just count them and remove or artificially add an
        // end tag to allow the parser to treat them as complete nodes for parsing
        // purposes.
        // Special handling for two variants of meta tag.
        //      without a trailing end of tag character : <meta>
        //      with a trailing end of tag character    : <meta/>
        // Replace both to have explicit start and end
        
        auto selfClosingTag = std::find_if(
                selfClosingTags.begin(), selfClosingTags.end(),
                    [&match](const std::string& t) { 
                        return (t.compare(match[2].str()) == 0);
                        }
            );
        bool isSelfClosing = selfClosingTag != selfClosingTags.end() && selfClosingTag->compare("")!=0;

        if( isSelfClosing ) {
            if (match[3].compare("/") != 0) { // e.g. <meta/> || <img />
                tag << match[2];
                thirdStage << "<" << tag.str() << ">"
                        << "</" << tag.str() << ">";
            }
            ++numNodes;
        }

        // Special handling for elements that are not part of the dom e.g <!DOCTYPE> 
        // <!--, <% etc.
        // These are not expected to have corresponding end tags i.e. /> or </ 
        // So we can either just count them and remove or artificially add an
        // end tag to allow the parser to treat them as complete nodes for parsing
        // purposes.
        else if ( match[2].compare("")==0 ){
            // Ignore tag
        }
        // Handling of all normal (non-comment, non-self closing tags)
        else {
            if (match[2].compare("div")==0)
                ++numDivNodes;
            tag << match[2] << match[4] << match[5];
            thirdStage << "<" << tag.str() << ">";
            ++numNodes;
        }
    }
#ifdef HTML_PARSER_DEBUG
    std::cout << std::endl << " Stage 3 Output" << std::endl << thirdStage.str() <<std::endl;
#endif
    // count leaf nodes - if next item in list has the same tag name preceded by closing tag
    std::stringstream fourthStage;
    std::regex leaf_capture("(<([a-zA-Z0-9]+)><(/)([a-zA-Z0-9]+)>)");
    auto thirdStageOut = thirdStage.str();
    thirdStage.clear();

        // At this point we assume that the html has been cleaned enough
        // that we are only left with tags such that each tag has an open
        // and close tag.
        //
    auto it3 = std::sregex_iterator(
        thirdStageOut.begin(), 
        thirdStageOut.end(), 
        leaf_capture);
    for (std::sregex_iterator i = it3; i != std::sregex_iterator(); ++i) {
        std::smatch match = *i;
        if (match[2].compare(match[4])==0 && match[3].compare("/")==0)
            ++numLeafNodes;
    }
    return {numNodes, numLeafNodes, numDivNodes, thirdStageOut}; 
}

// Url string  client::gNumNodes
std::tuple<uint64_t, uint64_t, uint64_t> parseHtml(int id, const std::string & storage) {
    
    // tuple to return with analysis statistics
    std::tuple<uint64_t, uint64_t, uint64_t> retVal = {id, 0U, 0U};
    client::gNumLeafNodes = 0;

    typedef client::mini_html_grammar<std::string::const_iterator> mini_html_grammar;
    
    mini_html_grammar root; // Grammar
    client::mini_html ast;  // AST tree

    using boost::spirit::ascii::space;
    std::string::const_iterator iter = storage.begin();
    std::string::const_iterator end = storage.end();
    if (storage.empty()) {
        std::cout << "-------------------------\n";
        std::cout << "Unable to parse URL\n";
        std::cout << "-------------------------\n";

        return retVal;
    }
    bool r = phrase_parse(iter, end, root, space, ast);
    
    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
        client::mini_html_printer printer;
        printer(ast);

        retVal = { client::gNumNodes, client::gNumLeafNodes, client::gDivNodeCount};

        std::cout << "Nodes = "     << std::get<0>(retVal)
                << "Leaf nodes = "  << std::get<1>(retVal)
                << "Leaf nodes = "  << std::get<2>(retVal)
                << std::endl;

        client::gNumNodes = 0;
        client::gNumLeafNodes = 0;
        client::gDivNodeCount = 0;
    }
    else
    {
        std::string::const_iterator some = iter+30;
        std::string context(iter, (some>end)?end:some);
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \": " << context << "...\"\n";
        std::cout << "-------------------------\n";
    }
    return retVal;
}

