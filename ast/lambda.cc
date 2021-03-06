#include "lambda.hpp"
#include "expression_def.hpp"
#include "utility.hpp"

using namespace std;
using namespace puppet::lexer;
using boost::optional;

namespace puppet { namespace ast {

    lambda::lambda()
    {
    }

    lambda::lambda(token_position position, optional<vector<parameter>> parameters, optional<vector<expression>> body) :
        _position(std::move(position)),
        _parameters(std::move(parameters)),
        _body(std::move(body))
    {
    }

    optional<vector<parameter>> const& lambda::parameters() const
    {
        return _parameters;
    }

    optional<vector<parameter>>& lambda::parameters()
    {
        return _parameters;
    }

    optional<vector<expression>> const& lambda::body() const
    {
        return _body;
    }

    optional<vector<expression>>& lambda::body()
    {
        return _body;
    }

    token_position const& lambda::position() const
    {
        return _position;
    }

    ostream& operator<<(ostream& os, ast::lambda const& lambda)
    {
        os << "|";
        pretty_print(os, lambda.parameters(), ", ");
        os << "| { ";
        pretty_print(os, lambda.body(), "; ");
        os << " }";
        return os;
    }

}}  // namespace puppet::ast
