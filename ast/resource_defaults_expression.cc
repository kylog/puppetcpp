#include "expression_def.hpp"
#include "utility.hpp"

using namespace std;
using namespace puppet::lexer;
using boost::optional;

namespace puppet { namespace ast {

    resource_defaults_expression::resource_defaults_expression()
    {
    }

    resource_defaults_expression::resource_defaults_expression(ast::type type, optional<vector<attribute_expression>> attributes) :
        _type(std::move(type)),
        _attributes(std::move(attributes))
    {
    }

    ast::type const& resource_defaults_expression::type() const
    {
        return _type;
    }

    ast::type& resource_defaults_expression::type()
    {
        return _type;
    }

    optional<vector<attribute_expression>> const& resource_defaults_expression::attributes() const
    {
        return _attributes;
    }

    optional<vector<attribute_expression>>& resource_defaults_expression::attributes()
    {
        return _attributes;
    }

    token_position const& resource_defaults_expression::position() const
    {
        return _type.position();
    }

    ostream& operator<<(ostream& os, resource_defaults_expression const& expr)
    {
        if (expr.type().name().empty()) {
            return os;
        }
        os << expr.type() << " { ";
        pretty_print(os, expr.attributes(), ", ");
        os << " }";
        return os;
    }

}}  // namespace puppet::ast

