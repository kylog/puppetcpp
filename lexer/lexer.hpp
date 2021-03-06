/**
 * @file
 * Declares the Puppet language lexer.
 * The lexer is responsible for turning an input source into a stream of tokens.
 * The tokens are consumed by the Puppet language grammar.
 */
#pragma once

#include <string>
#include <locale>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <exception>
#include <tuple>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/spirit/include/lex_lexer.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_lexertl_token.hpp>
#include <boost/spirit/include/lex_static_lexertl.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>

namespace puppet { namespace lexer {
    /**
     * The lexer tab width.
     * This is how many spaces the lexer considers a tab to be.
     */
    constexpr const int LEXER_TAB_WIDTH = 4;

    /**
     * Exception for lexer errors.
     * @tparam Iterator The location iterator type.
     */
    template <typename Iterator>
    struct lexer_exception : std::runtime_error
    {
        /**
         * Constructs a lexer exception.
         * @param location The location where lexing failed.
         * @param message The lexer exception message.
         */
        lexer_exception(Iterator location, std::string const& message) :
            std::runtime_error(message),
            _location(location)
        {
        }

        /**
         * Gets the location where lexing failed.
         * @return Returns the location where lexing failed.
         */
        Iterator const& location() const
        {
            return _location;
        }

     private:
        Iterator _location;
    };

    /**
     * Stores the offset in input stream and line for a token.
     */
    typedef std::tuple<std::size_t, std::size_t> token_position;

    /**
     * Lexer iterator type used to support heredoc parsing.
     * Heredocs require a more complicated iterator type due to the fact heredoc lines are parsed out-of-order.
     * This iterator supports skipping over lines that have already been parsed for a heredoc token.
     * This iterator also keeps track of position in the input.
     * @tparam Iterator The underlying iterator type.
     */
    template <class Iterator>
    struct lexer_iterator : boost::iterator_adaptor<lexer_iterator<Iterator>, Iterator>
    {
        /**
         * Default constructor for lexer_iterator.
         */
        lexer_iterator() :
            _position { 0, 0 }
        {
        }

        /**
         * Type conversion from the underlying iterator type.
         * @param iter The underlying iterator instance.
         */
        explicit lexer_iterator(Iterator const& iter) :
            boost::iterator_adaptor<lexer_iterator<Iterator>, Iterator>(iter),
            _position { 0, 1 }
        {
        }

        /**
         * Gets the position of the token.
         * @return Returns the position of the token.
         */
        token_position const& position() const
        {
            return _position;
        }

    private:
        friend class boost::iterator_core_access;

        template <typename T> friend
        struct lexer;

        void set_next(lexer_iterator<Iterator> const& next)
        {
            // Set the next underlying iterator
            _next_iter = next.base_reference();
            _next_position = next._position;
        }

        bool get_next(lexer_iterator<Iterator>& next) const
        {
            if (!_next_iter) {
                return false;
            }

            // Return the next iterator data
            next.base_reference() = *_next_iter;
            next._position = _next_position;
            return true;
        }

        void increment()
        {
            auto& base = this->base_reference();

            auto current = *base;

            // If there is a next and we've reach the end of the line, "skip" to next
            // This will effectively skip over any heredoc lines that were parsed
            if (_next_iter && current == '\n') {
                base = *_next_iter;
                _next_iter = boost::none;

                _position = _next_position;
                _next_position = token_position();
            } else {
                // Otherwise, check for a new line and increment the line counter
                if (current == '\n') {
                    ++std::get<1>(_position);
                }

                ++std::get<0>(_position);
                ++base;
            }
        }

        token_position _position;
        boost::optional<Iterator> _next_iter;
        token_position _next_position;
    };

    /**
     * Represents the kinds of tokens returned by the lexer.
     * Every token returned from the lexer will either have one of these id values or be less than 128.
     * If the id is less than 128, the token represents a literal character token.
     */
    enum class token_id
    {
        unknown = boost::spirit::lex::min_token_id,
        append, // Not supported in grammar, but kept for legacy parsing
        remove, // Not supported in grammer, but kept for legacy parsing
        equals,
        not_equals,
        match,
        not_match,
        greater_equals,
        less_equals,
        fat_arrow,
        plus_arrow,
        left_shift,
        left_double_collect,
        left_collect,
        right_shift,
        right_double_collect,
        right_collect,
        atat,
        in_edge,
        in_edge_sub,
        out_edge,
        out_edge_sub,
        keyword_case,
        keyword_class,
        keyword_default,
        keyword_define,
        keyword_if,
        keyword_elsif,
        keyword_else,
        keyword_inherits,
        keyword_node,
        keyword_and,
        keyword_or,
        keyword_undef,
        keyword_false,
        keyword_true,
        keyword_in,
        keyword_unless,
        keyword_function,
        keyword_type,
        keyword_attr,
        keyword_private,
        statement_call,
        single_quoted_string,
        double_quoted_string,
        bare_word,
        variable,
        type,
        name,
        regex,
        heredoc,
        number,
        array_start,        // Same as '[', but whitespace delimited to force array expression
        comment,            // Not in token stream
        whitespace,         // Not in token stream
        unclosed_quote      // Error token that will not match any grammars
    };

    /**
     * Stream insertion operator for token id.
     * @param os The output stream to write the token id.
     * @param id The token id to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, token_id const& id);
}}  // namespace puppet::lexer

namespace boost { namespace spirit { namespace qi {
    /** @cond NOT_DOCUMENTED */
    /**
     * Specialization for information for tokens with id type token_id.
     * @tparam Context The type of context.
     * @return Returns The boost info for the token.
     */
    template <>
    template <typename Context>
    info plain_token<puppet::lexer::token_id>::what(Context&) const
    {
        return boost::spirit::info("token", boost::lexical_cast<std::string>(id));
    }

    /**
     * Specialization for information for tokens with id type char.
     * @tparam Context The type of context.
     * @return Returns The boost info for the token.
     */
    template <>
    template <typename Context>
    info plain_token<char>::what(Context&) const
    {
        return boost::spirit::info("token", boost::lexical_cast<std::string>(static_cast<puppet::lexer::token_id>(id)));
    }
    /**
     * Specialization for information for tokens with id type token_id.
     * @tparam Context The type of context.
     * @return Returns The boost info for the token.
     */
    template <>
    template <typename Context>
    info plain_raw_token<puppet::lexer::token_id>::what(Context&) const
    {
        return boost::spirit::info("raw_token", boost::lexical_cast<std::string>(id));
    }

    /**
     * Specialization for information for tokens with id type char.
     * @tparam Context The type of context.
     * @return Returns The boost info for the token.
     */
    template <>
    template <typename Context>
    info plain_raw_token<char>::what(Context&) const
    {
        return boost::spirit::info("raw_token", boost::lexical_cast<std::string>(static_cast<puppet::lexer::token_id>(id)));
    }
    /** @endcond */
}}}  // namespace boost::spirit::qi

namespace puppet { namespace lexer {
    /**
     * Represents data about a string token.
     * Used for heredocs and quoted strings.
     */
    struct string_token
    {
        string_token() {}
        /**
         * Constructs a string token.
         * @param position The position in the input source for the token.
         * @param text The text for the string.
         * @param format The format for the string (heredoc only).
         * @param interpolated True if the string should be interpolated or false if not.
         * @param escaped True if the $ character should be escaped or false if not.
         */
        string_token(
            token_position position,
            std::string text,
            std::string format = std::string(),
            bool interpolated = true,
            bool escaped = true) :
                _position(std::move(position)),
                _text(std::move(text)),
                _format(std::move(format)),
                _interpolated(interpolated),
                _escaped(escaped)
        {
        }

        /**
         * Gets the position of the token.
         * @return Returns the position of the token.
         */
        token_position const& position() const
        {
            return _position;
        }

        /**
         * Gets the text of the string token.
         * @return Returns the text of the string token.
         */
        std::string const& text() const
        {
            return _text;
        }

        /**
         * Gets the text of the string token.
         * @return Returns the text of the string token.
         */
        std::string& text()
        {
            return _text;
        }

        /**
         * Gets the format of the string token.
         * An empty format means "regular string".
         * @return Returns the format of the string token.
         */
        std::string const& format() const
        {
            return _format;
        }

        /**
         * Gets the format of the string token.
         * An empty format means "regular string".
         * @return Returns the format of the string token.
         */
        std::string& format()
        {
            return _format;
        }

        /**
         * Gets whether or not the string should be interpolated.
         * @return Returns whether or not the string should be interpolated.
         */
        bool interpolated() const
        {
            return _interpolated;
        }

        /**
         * Gets whether or not the interpolation character ($) should be escaped.
         * @return Returns whether or not the interpolation character ($) should be escaped.
         */
        bool escaped() const
        {
            return _escaped;
        }

     private:
        token_position _position;
        std::string _text;
        std::string _format;
        bool _interpolated;
        bool _escaped;
    };

    /**
     * Stream insertion operator for string token.
     * @param os The output stream to write the token.
     * @param token The token to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, string_token const& token);

}}  // namespace puppet::lexer

namespace boost { namespace spirit { namespace traits
{
    /**
     * Utility for converting iterator ranges into string tokens.
     */
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<puppet::lexer::string_token, Iterator>
    {
        /**
         * Assigns a string token based on an iterator range.
         * String tokens cannot be assigned from iterator range, only from values.
         * Calling this function will result in a runtime error.
         * @param first The first iterator.
         * @param last The last iterator.
         * @param attr The resulting string token attribute.
         */
        static void call(Iterator const& first, Iterator const& last, puppet::lexer::string_token& attr)
        {
            // This should not get called and only exists for the code to compile
            // Tokens that have string data associated with them should be assigned by value, not interators
            throw std::runtime_error("attempt to assign string token from iterators.");
        }
    };
}}}  // namespace boost::spirit::traits

namespace puppet { namespace lexer {
    /**
     * Implements the lexer for the Puppet language.
     * The lexer is responsible for producing a stream of tokens for parsing.
     * @tparam Lexer The base lexer type.
     */
    template <typename Lexer>
    struct lexer : boost::spirit::lex::lexer<Lexer>
    {
        /**
        * The base type for this lexer type.
        */
        typedef typename boost::spirit::lex::lexer<Lexer> base_type;

        /**
         * The type of token this lexer produces.
         */
        typedef typename Lexer::token_type token_type;

       /**
         * The token id type.
         */
        typedef typename Lexer::id_type id_type;

        /**
         * The type of iterator for the output token stream.
         */
        typedef typename Lexer::iterator_type iterator_type;

        /**
         * The type of character for the input stream.
         */
        typedef typename Lexer::char_type char_type;

        /**
         * The type of string for the input stream.
         */
        typedef std::basic_string<char_type> string_type;

        /**
         * Constructs a new lexer.
         */
        lexer() :
            single_quoted_string("'([^\\\\']|\\\\\\\\|\\\\.)*'",        static_cast<id_type>(token_id::single_quoted_string)),
            double_quoted_string("\\\"([^\\\\\"]|\\\\\\\\|\\\\.)*\\\"", static_cast<id_type>(token_id::double_quoted_string)),
            heredoc(HEREDOC_PATTERN,                                    static_cast<id_type>(token_id::heredoc))
        {
            namespace lex = boost::spirit::lex;

            // The following are lexer states that are used to parse regular expressions.
            // This solves the ambiguity between having multiple division operators on a single line (e.g. "1 / 2 / 3")
            // and parsing a regex; without this, "/ 2 /" above would parse as a regex token.
            // For SLASH_CHECK_STATE, we're doing a lookahead to see if the next token should be a division operator.
            // For FORCE_SLASH_STATE, the lookahead succeeded, so force the next token to be "/" and not a regex, and
            // reset state back to the initial state.
            this->self(SLASH_CHECK_STATE).add(SLASH_CHECK_PATTERN, '/');
            this->self(FORCE_SLASH_STATE, base_type::initial_state()) = lex::token_def<>(SLASH_CHECK_PATTERN, '/') [ use_last ];

            // The following are in match order
            // Add the three-character operators
            this->self.add
                ("<<\\|", static_cast<id_type>(token_id::left_double_collect));
            this->self +=
                lex::token_def<>("\\|>>", static_cast<id_type>(token_id::right_double_collect)) [ no_regex ];

            // Add the two-character operators
            this->self.add
                ("\\+=", static_cast<id_type>(token_id::append))
                ("-=",   static_cast<id_type>(token_id::remove))
                ("==",   static_cast<id_type>(token_id::equals))
                ("!=",   static_cast<id_type>(token_id::not_equals))
                ("=~",   static_cast<id_type>(token_id::match))
                ("!~",   static_cast<id_type>(token_id::not_match))
                (">=",   static_cast<id_type>(token_id::greater_equals))
                ("<=",   static_cast<id_type>(token_id::less_equals))
                ("=>",   static_cast<id_type>(token_id::fat_arrow))
                ("\\+>", static_cast<id_type>(token_id::plus_arrow))
                ("<<",   static_cast<id_type>(token_id::left_shift))
                ("<\\|", static_cast<id_type>(token_id::left_collect))
                (">>",   static_cast<id_type>(token_id::right_shift))
                ("@@",   static_cast<id_type>(token_id::atat))
                ("->",   static_cast<id_type>(token_id::in_edge))
                ("~>",   static_cast<id_type>(token_id::in_edge_sub))
                ("<-",   static_cast<id_type>(token_id::out_edge))
                ("<~",   static_cast<id_type>(token_id::out_edge_sub));
            this->self +=
                lex::token_def<>("\\|>", static_cast<id_type>(token_id::right_collect)) [ no_regex ];

            // Add single character operators
            // The ids for these tokens are the characters themselves
            this->self += lex::token_def<>('[') |
                 lex::token_def<>(']') [ no_regex ] |
                '{' |
                '}' |
                '(' |
                 lex::token_def<>(')') [ no_regex ] |
                '=' |
                '>' |
                '<' |
                '+' |
                '-' |
                '/' |
                '*' |
                '%' |
                '!' |
                '.' |
                '|' |
                '@' |
                ':' |
                ',' |
                ';' |
                '?' |
                '~';

            // Add the keywords
            this->self.add
                ("case",     static_cast<id_type>(token_id::keyword_case))
                ("class",    static_cast<id_type>(token_id::keyword_class))
                ("default",  static_cast<id_type>(token_id::keyword_default))
                ("define",   static_cast<id_type>(token_id::keyword_define))
                ("if",       static_cast<id_type>(token_id::keyword_if))
                ("elsif",    static_cast<id_type>(token_id::keyword_elsif))
                ("else",     static_cast<id_type>(token_id::keyword_else))
                ("inherits", static_cast<id_type>(token_id::keyword_inherits))
                ("node",     static_cast<id_type>(token_id::keyword_node))
                ("and",      static_cast<id_type>(token_id::keyword_and))
                ("or",       static_cast<id_type>(token_id::keyword_or))
                ("undef",    static_cast<id_type>(token_id::keyword_undef))
                ("in",       static_cast<id_type>(token_id::keyword_in))
                ("unless",   static_cast<id_type>(token_id::keyword_unless))
                ("function", static_cast<id_type>(token_id::keyword_function))
                ("type",     static_cast<id_type>(token_id::keyword_type))
                ("attr",     static_cast<id_type>(token_id::keyword_attr))
                ("private",  static_cast<id_type>(token_id::keyword_private));

            // Add the statement calls
            this->self.add
                ("require", static_cast<id_type>(token_id::statement_call))
                ("realize", static_cast<id_type>(token_id::statement_call))
                ("include", static_cast<id_type>(token_id::statement_call))
                ("contain", static_cast<id_type>(token_id::statement_call))
                ("tag",     static_cast<id_type>(token_id::statement_call))
                ("debug",   static_cast<id_type>(token_id::statement_call))
                ("info",    static_cast<id_type>(token_id::statement_call))
                ("notice",  static_cast<id_type>(token_id::statement_call))
                ("warning", static_cast<id_type>(token_id::statement_call))
                ("error",   static_cast<id_type>(token_id::statement_call))
                ("fail",    static_cast<id_type>(token_id::statement_call))
                ("import",  static_cast<id_type>(token_id::statement_call));

            this->self +=
                lex::token_def<>("true",  static_cast<id_type>(token_id::keyword_true)) [ no_regex ] |
                lex::token_def<>("false", static_cast<id_type>(token_id::keyword_false)) [ no_regex ];

            // Variables, bare words, numbers, class references, names, regexes, strings, comments, and whitespace
            this->self +=
                lex::token_def<>("\\s+\\[",                                                       static_cast<id_type>(token_id::array_start)) [ use_last ] |
                lex::token_def<>("(0[xX][0-9A-Fa-f]+)|(0[0-7]+)|(0?\\d+(\\.\\d+)?([eE]-?\\d+)?)", static_cast<id_type>(token_id::number))      [ no_regex ] |
                lex::token_def<>("((::){0,1}[A-Z][\\w]*)+",                                       static_cast<id_type>(token_id::type))        [ no_regex ] |
                lex::token_def<>("((::)?[a-z][\\w]*)(::[a-z][\\w]*)*",                            static_cast<id_type>(token_id::name))        [ no_regex ] |
                lex::token_def<>("[a-z_]([\\w\\-]*[\\w])?",                                       static_cast<id_type>(token_id::bare_word))   [ no_regex ] |
                lex::token_def<>("(\\/\\/)|(\\/[^*][^/\\n]*\\/)",                                 static_cast<id_type>(token_id::regex))       [ no_regex ] |
                single_quoted_string                                                                                                           [ parse_single_quoted_string ] |
                double_quoted_string                                                                                                           [ parse_double_quoted_string ] |
                heredoc                                                                                                                        [ parse_heredoc ] |
                lex::token_def<>("(#[^\\n]*)|(\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/)",              static_cast<id_type>(token_id::comment))     [ lex::_pass = lex::pass_flags::pass_ignore ] |
                lex::token_def<>("\\s+",                                                          static_cast<id_type>(token_id::whitespace))  [ lex::_pass = lex::pass_flags::pass_ignore ];
            this->self.add
                ("\\$(::)?(\\w+::)*\\w+",   static_cast<id_type>(token_id::variable));

            // Lastly, a catch for unclosed quotes and unknown tokens
            this->self.add
                ("['\"]", static_cast<id_type>(token_id::unclosed_quote))
                (".",     static_cast<id_type>(token_id::unknown));
        }

        /**
         * The token representing single quoted strings.
         */
        boost::spirit::lex::token_def<string_token> single_quoted_string;

        /**
         * The token representing double quoted strings.
         */
        boost::spirit::lex::token_def<string_token> double_quoted_string;

        /**
         * The token representing heredocs.
         */
        boost::spirit::lex::token_def<string_token> heredoc;

    private:
        typedef typename token_type::iterator_type input_iterator_type;
        typedef typename iterator_type::shared_functor_type context_type;

        static string_type extract_string(input_iterator_type const& start, input_iterator_type const& end, string_type const& escapes = string_type(), bool warn = true, int margin = 0)
        {
            std::basic_ostringstream<char_type> ss;

            int current_margin = margin;
            for (auto it = start; it != end; ++it) {
                // This logic handles heredocs with margin specifiers (margin > 0)
                for (; current_margin > 0 && it != end; ++it) {
                    // If we've found a non-whitespace character, we're done
                    if (*it != ' ' && *it != '\t') {
                        break;
                    }
                    // If we've found a tab, decrement by the tab width
                    if (*it == '\t') {
                        current_margin -= (current_margin > LEXER_TAB_WIDTH ? LEXER_TAB_WIDTH : current_margin);
                    } else {
                        current_margin -= 1;
                    }
                }
                if (it == end) {
                    break;
                }

                // No more margin for this line
                current_margin = 0;

                // Perform escape replacements
                // Note: $ character escaping is handled when a string is interpolated
                if (*it == '\\' && !escapes.empty()) {
                    auto next = it;
                    ++next;
                    if (next != end && *next == '\r') {
                        ++next;
                    }
                    if (next != end && escapes.find(*next) != string_type::npos) {
                        switch (*next) {
                            case 'r':
                                ss << '\r';
                                break;

                            case 'n':
                                ss << '\n';
                                break;

                            case 't':
                                ss << '\t';
                                break;

                            case 's':
                                ss << ' ';
                                break;

                            case 'u':
                                // TODO: implement unicode escape sequences
                                throw lexer_exception<input_iterator_type>(it, "unicode escape sequences are not yet supported.");
                                break;

                            case '\n':
                                // Treat as new line, so reset the margin
                                current_margin = margin;
                                break;

                            default:
                                ss << *next;
                                break;
                        }
                        it = next;
                        continue;
                    } else if (next != end) {
                        if (warn && *next != '$') {
                            // TODO: warn of invalid sequence instead of throwing
                            throw lexer_exception<input_iterator_type>(it, (boost::format("unexpected escape sequence \\%1%.") % *next).str());
                        }
                    }
                } else if (*it == '\n') {
                    // Reset the margin
                    current_margin = margin;
                }

                ss << *it;
            }
            return ss.str();
        }

        static void parse_heredoc(input_iterator_type const& start, input_iterator_type& end, boost::spirit::lex::pass_flags& matched, id_type& id, context_type& context)
        {
            static boost::regex pattern(HEREDOC_PATTERN);

            // Force any following '/' to be interpreted as a '/' token
            force_slash(context);

            // Helper functions
            static auto is_space = [](char c) { return c == ' ' || c == '\t'; };
            static auto throw_not_found = [](input_iterator_type const& location, std::string const& tag) {
                throw lexer_exception<input_iterator_type>(location, (boost::format("unexpected end of input while looking for heredoc end tag '%1%'.") % tag).str());
            };
            static auto move_next_line = [](input_iterator_type& begin, input_iterator_type const& end) -> bool {
                for (; begin != end && *begin != '\n'; ++begin);
                if (begin == end) {
                    return false;
                }

                // Move past the newline
                ++begin;
                return true;
            };

            // Boost.Regex needs bi-directional iterators, so we need to copy the token range (just the @(...) part)
            string_type token(start, end);

            // Extract the tag, format, and escapes from the token
            boost::match_results<typename string_type::const_iterator> match;
            if (!boost::regex_match(token, match, pattern) || match.size() != 6) {
                throw lexer_exception<input_iterator_type>(start, "unexpected heredoc format.");
            }

            // Trim the tag
            string_type tag(match[1].first, match[1].second);
            boost::trim(tag);

            // Check for interpolation
            bool interpolated = false;
            bool escaped = true;
            if (boost::starts_with(tag, "\"") && boost::ends_with(tag, "\"")) {
                interpolated = true;
                boost::trim_if(tag, boost::is_any_of("\""));
            }

            // Check for optional format
            string_type format;
            if (match[2].first != match[2].second) {
                format.assign(match[3].first, match[3].second);
            }

            // Check for optional escapes
            string_type escapes;
            if (match[4].first != match[4].second) {
                escapes.assign(match[5].first, match[5].second);
                if (escapes.empty()) {
                    // Enable all heredoc escapes
                    escapes = HEREDOC_ESCAPES;
                } else {
                    // Verify the escapes
                    if (!boost::all(escapes, boost::is_any_of(HEREDOC_ESCAPES))) {
                        throw lexer_exception<input_iterator_type>(start, (boost::format("invalid heredoc escapes \"%1%\": only t, r, n, s, u, L, and $ are allowed.") % escapes).str());
                    }
                    // TODO: verify uniqueness of each character (i.e. is this really important)?

                    // If the list contains $, we'll escape it when the string gets interpolated, so remove it now
                    if (boost::contains(escapes, "$")) {
                        boost::replace_all(escapes, "$", "");
                    } else {
                        escaped = false;
                    }
                }
                boost::replace_all(escapes, "L", "\n");
                escapes += "\\";
            }

            auto& eoi = context.get_eoi();

            // Move to the next line to process, skipping over any previous heredoc on the token's line
            input_iterator_type doc_begin;
            if (!start.get_next(doc_begin)) {
                doc_begin = end;
                if (!move_next_line(doc_begin, eoi)) {
                    throw_not_found(start, tag);
                }
            }

            bool remove_break = false;
            bool has_margin = false;
            int margin = 0;
            auto doc_end = doc_begin;

            // Search for the end tag
            while (doc_end != eoi) {
                auto line_end = doc_end;
                for (; line_end != end && is_space(*line_end); ++line_end) {
                    margin += (*line_end == ' ') ? 1 : LEXER_TAB_WIDTH;
                }
                if (line_end == eoi) {
                    throw_not_found(start, tag);
                }
                if (*line_end == '|') {
                    has_margin = true;
                    for (++line_end; line_end != end && is_space(*line_end); ++line_end);
                }
                if (line_end == eoi) {
                    throw_not_found(start, tag);
                }
                if (*line_end == '-') {
                    remove_break = true;
                    for (++line_end; line_end != end && is_space(*line_end); ++line_end);
                }
                if (line_end == eoi) {
                    throw_not_found(start, tag);
                }

                // Look for the end tag
                auto search_it = tag.begin();
                for (; line_end != eoi && search_it != tag.end() && *search_it == *line_end; ++search_it, ++line_end);
                if (search_it == tag.end()) {
                    // Possibly found the tag; ensure the remainder of the line is whitespace
                    for (; line_end != eoi && is_space(*line_end); ++line_end);
                    if (line_end != eoi && *line_end == '\r') {
                        ++line_end;
                    }
                    if (line_end == eoi || *line_end == '\n') {
                        break;
                    }

                    // Not found
                }

                // Move to the next line
                move_next_line(line_end, eoi);
                doc_end = line_end;
                margin = 0;
            }

            if (doc_end == eoi) {
                throw_not_found(start, tag);
            }

            auto next = doc_end;
            move_next_line(next, eoi);
            end.set_next(next);

            // Extract the heredoc text from the iterators, handling margin and escape characters
            auto text = extract_string(doc_begin, doc_end, escapes, true, has_margin ? margin : 0);

            // Remove the trailing line break if instructed to do so
            if (remove_break) {
                if (boost::ends_with(text, "\n")) {
                    text.pop_back();
                }
                if (boost::ends_with(text, "\r")) {
                    text.pop_back();
                }
            }

            context.set_value(string_token(start.position(), text, format, interpolated, escaped));
        }

        static void parse_single_quoted_string(input_iterator_type const& start, input_iterator_type const& end, boost::spirit::lex::pass_flags& matched, id_type& id, context_type& context)
        {
            // Force any following '/' to be interpreted as a '/' token
            force_slash(context);

            auto text = extract_string(start, end, "\\'", true);
            boost::trim_if(text, boost::is_any_of("'"));
            context.set_value(string_token(start.position(), text, {}, false));
        }

        static void parse_double_quoted_string(input_iterator_type const& start, input_iterator_type const& end, boost::spirit::lex::pass_flags& matched, id_type& id, context_type& context)
        {
            // Force any following '/' to be interpreted as a '/' token
            force_slash(context);

            // Don't include $ in the escape list; it'll be handled during interpolation
            auto text = extract_string(start, end, "\\\"'nrtsu", true);
            boost::trim_if(text, boost::is_any_of("\""));
            context.set_value(string_token(start.position(), text, {}, true));
        }

        static void no_regex(input_iterator_type const& start, input_iterator_type const& end, boost::spirit::lex::pass_flags& matched, id_type& id, context_type& context)
        {
            // Force any following '/' to be interpreted as a '/' token
            force_slash(context);
        }

        static void force_slash(context_type& context)
        {
            // If the next token is /, then set the "no regex" state
            // This will force the next '/' to match as '/' and not the start of a regex
            if (!context.lookahead('/', context.get_state_id(SLASH_CHECK_STATE))) {
                return;
            }
            context.set_state(context.get_state_id(FORCE_SLASH_STATE));
        }

        static void use_last(input_iterator_type& start, input_iterator_type const& end, boost::spirit::lex::pass_flags& matched, id_type& id, context_type& context)
        {
            // Use the last character in the range
            auto last = start;
            for (auto current = start; current != end; ++current) {
                last = current;
            }
            start = last;
        }

        static const char* const HEREDOC_PATTERN;
        static const char* const HEREDOC_ESCAPES;
        static const char* const FORCE_SLASH_STATE;
        static const char* const SLASH_CHECK_STATE;
        static const char* const SLASH_CHECK_PATTERN;
    };

    template<typename Lexer>
    char const* const lexer<Lexer>::HEREDOC_PATTERN = "@\\(\\s*([^):/\\r\\n]+)\\s*(:\\s*([a-z][a-zA-Z0-9_+]+))?\\s*(\\/\\s*([\\w|$]*)\\s*)?\\)";
    template<typename Lexer>
    char const* const lexer<Lexer>::HEREDOC_ESCAPES = "trnsuL$";
    template<typename Lexer>
    char const* const lexer<Lexer>::FORCE_SLASH_STATE = "FS";
    template<typename Lexer>
    char const* const lexer<Lexer>::SLASH_CHECK_STATE = "SC";
    template<typename Lexer>
    char const* const lexer<Lexer>::SLASH_CHECK_PATTERN = "\\s*(\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/\\s*)*\\/";

    /**
     * The input iterator for files.
     */
    typedef lexer_iterator<boost::spirit::multi_pass<std::istreambuf_iterator<char>>> lexer_istreambuf_iterator;
    /**
     * The input iterator for strings.
     */
    typedef lexer_iterator<typename std::string::const_iterator> lexer_string_iterator;

    /**
     * The token type for the lexer.
     * @tparam Iterator The input iterator for the token.
     */
	template <typename Iterator>
	using lexer_token = boost::spirit::lex::lexertl::token<Iterator, boost::mpl::vector<string_token>>;

    /**
     * The lexer to use for files.
     */
    typedef lexer<boost::spirit::lex::lexertl::actor_lexer<lexer_token<lexer_istreambuf_iterator>>>        file_lexer;
    /**
     * The static lexer to use for files.
     * Include "static_lexer.hpp" before including "lexer.hpp" to use this type.
     */
    typedef lexer<boost::spirit::lex::lexertl::static_actor_lexer<lexer_token<lexer_istreambuf_iterator>>> file_static_lexer;
    /**
     * The lexer to use for strings.
     */
    typedef lexer<boost::spirit::lex::lexertl::actor_lexer<lexer_token<lexer_string_iterator>>>            string_lexer;
    /**
     * The static lexer to use for strings.
     * Include "static_lexer.hpp" before including "lexer.hpp" to use this type.
     */
    typedef lexer<boost::spirit::lex::lexertl::static_actor_lexer<lexer_token<lexer_string_iterator>>>     string_static_lexer;

    /**
     * Gets the lexer's beginning iterator for the given file.
     * @param file The file to lex.
     * @return Returns the beginning input interator for the lexer.
     */
    lexer_istreambuf_iterator lex_begin(std::ifstream& file);

    /**
     * Gets the lexer's ending iterator for the given file.
     * @param file The file to lex.
     * @return Returns the ending input interator for the lexer.
     */
    lexer_istreambuf_iterator lex_end(std::ifstream& file);

    /**
     * Gets the lexer's beginning iterator for the given string.
     * @param str The string to lex.
     * @return Returns the beginning input interator for the lexer.
     */
    lexer_string_iterator lex_begin(std::string const& str);

    /**
     * Gets the lexer's ending iterator for the given string.
     * @param str The string to lex.
     * @return Returns the ending input interator for the lexer.
     */
    lexer_string_iterator lex_end(std::string const& str);

    /**
     * Gets the line of text and column for the given position in a file.
     * @param fs The file stream of the file.
     * @param position The position inside the file.
     * @param tab_width Specifies the width of a tab character for column calculations.
     * @return Returns a tuple of the line in the file and the column of the position.
     */
    std::tuple<std::string, std::size_t> get_line_and_column(std::ifstream& fs, std::size_t position, std::size_t tab_width = LEXER_TAB_WIDTH);

    /**
     * Utility type for visiting tokens for position and line information.
     * @tparam Token The type of token.
     */
    template <typename Token>
    struct token_position_visitor : boost::static_visitor<token_position>
    {
        /**
         * Called for tokens that contain iterator ranges.
         * @param range The iterator range of the token.
         * @return Returns the tuple of position and line.
         */
        template <typename Iterator>
        result_type operator()(boost::iterator_range<Iterator> const& range) const
        {
            return range.begin().position();
        }

        /**
         * Called for string tokens.
         * @param token The string token.
         * @return Returns the tuple of position and line.
         */
        result_type operator()(string_token const& token) const
        {
            return token.position();
        }
    };

    /**
     * Gets the given token's position.
     * @tparam Token The type of token.
     * @param file The file containing the token, used if the token is the EOI token.
     * @param token The token to get the position for.
     * @return Returns the token's position.
     */
    template <typename Token>
    token_position get_position(std::ifstream& file, Token const& token)
    {
        if (token == Token()) {
            // The token is the EOI token
            // HACK: since we have no position information for this token, we read the entire file to count new lines
            // and find the last position in the file
            auto pos = file.tellg();
            file.seekg(0);

            std::size_t position = 0, lines = 1;
            bool last_is_newline = false;
            for (std::istreambuf_iterator<char> it(file), end; it != end; ++it) {
                last_is_newline = *it == '\n';
                if (last_is_newline) {
                    ++lines;
                }
                ++position;
            }

            // If the last character in the file is a newline, point at the character before it
            if (last_is_newline) {
                --lines;
                --position;
            }

            file.seekg(pos);
            return std::make_tuple(position, lines);
        }
        return boost::apply_visitor(token_position_visitor<Token>(), token.value());
    }

}}  // namespace puppet::lexer
