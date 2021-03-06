#include "access_expression.hpp"
#include "expression_def.hpp"
#include "utility.hpp"

using namespace std;
using namespace puppet::lexer;
using boost::optional;

namespace puppet { namespace ast {

    access::access()
    {
    }

    access::access(token_position position, vector<expression> arguments) :
        _position(std::move(position)),
        _arguments(std::move(arguments))
    {
    }

    vector<expression> const& access::arguments() const
    {
        return _arguments;
    }

    vector<expression>& access::arguments()
    {
        return _arguments;
    }

    token_position const& access::position() const
    {
        return _position;
    }

    ostream& operator<<(ostream& os, access const& access)
    {
        os << '[';
        pretty_print(os, access.arguments(), ", ");
        os << ']';
        return os;
    }

    access_expression::access_expression()
    {
    }

    access_expression::access_expression(primary_expression target, vector<access> accesses) :
        _target(std::move(target)),
        _accesses(std::move(accesses))
    {
    }

    primary_expression const& access_expression::target() const
    {
        return _target;
    }

    primary_expression& access_expression::target()
    {
        return _target;
    }

    vector<access> const& access_expression::accesses() const
    {
        return _accesses;
    }

    vector<access>& access_expression::accesses()
    {
        return _accesses;
    }

    token_position const& access_expression::position() const
    {
        return _target.position();
    }

    ostream& operator<<(ostream& os, access_expression const& expr)
    {
        if (expr.target().blank() || expr.accesses().empty()) {
            return os;
        }
        os << expr.target();
        for (auto const& access : expr.accesses()) {
            os << access;
        }
        return os;
    }

}}  // namespace puppet::ast
