#include "expression_def.hpp"

using namespace std;
using namespace puppet::lexer;

namespace puppet { namespace ast {

    ostream& operator<<(ostream& os, unary_operator op)
    {
        switch (op) {
            case unary_operator::none:
                break;

            case unary_operator::negate:
                os << '-';
                break;

            case unary_operator::logical_not:
                os << '!';
                break;

            case unary_operator::splat:
                os << '*';
                break;

            default:
                throw runtime_error("invalid unary operator.");
        }
        return os;
    }

    unary_expression::unary_expression() :
        _op(unary_operator::none)
    {
    }

    unary_expression::unary_expression(token_position position, unary_operator op, primary_expression operand) :
        _position(std::move(position)),
        _op(op),
        _operand(std::move(operand))
    {
    }

    unary_operator unary_expression::op() const
    {
        return _op;
    }

    unary_operator& unary_expression::op()
    {
        return _op;
    }

    primary_expression const& unary_expression::operand() const
    {
        return _operand;
    }

    primary_expression& unary_expression::operand()
    {
        return _operand;
    }

    token_position const& unary_expression::position() const
    {
        return _position;
    }

    ostream& operator<<(ostream& os, unary_expression const& expr)
    {
        if (expr.op() == unary_operator::none) {
            os << expr.operand();
            return os;
        }
        os << expr.op() << expr.operand();
        return os;
    }

    ostream& operator<<(ostream& os, binary_operator op)
    {
        switch (op) {
            case binary_operator::none:
                break;

            case binary_operator::in:
                os << "in";
                break;
            case binary_operator::match:
                os << "=~";
                break;
            case binary_operator::not_match:
                os << "!~";
                break;
            case binary_operator::multiply:
                os << "*";
                break;
            case binary_operator::divide:
                os << "/";
                break;
            case binary_operator::modulo:
                os << "%";
                break;
            case binary_operator::plus:
                os << "+";
                break;
            case binary_operator::minus:
                os << "-";
                break;
            case binary_operator::left_shift:
                os << "<<";
                break;
            case binary_operator::right_shift:
                os << ">>";
                break;
            case binary_operator::equals:
                os << "==";
                break;
            case binary_operator::not_equals:
                os << "!=";
                break;
            case binary_operator::greater_than:
                os << ">";
                break;
            case binary_operator::greater_equals:
                os << ">=";
                break;
            case binary_operator::less_than:
                os << "<";
                break;
            case binary_operator::less_equals:
                os << "<=";
                break;
            case binary_operator::logical_and:
                os << "and";
                break;
            case binary_operator::logical_or:
                os << "or";
                break;
            case binary_operator::assignment:
                os << '=';
                break;
            case binary_operator::in_edge:
                os << "->";
                break;
            case binary_operator::in_edge_subscribe:
                os << "~>";
                break;
            case binary_operator::out_edge:
                os << "<-";
                break;
            case binary_operator::out_edge_subscribe:
                os << "<~";
                break;
            default:
                throw runtime_error("invalid binary operator.");
        }
        return os;
    }

    binary_expression::binary_expression() :
        _op(binary_operator::none)
    {
    }

    binary_expression::binary_expression(binary_operator op, primary_expression operand) :
        _op(op),
        _operand(std::move(operand))
    {
    }

    binary_operator binary_expression::op() const
    {
        return _op;
    }

    binary_operator& binary_expression::op()
    {
        return _op;
    }

    primary_expression const& binary_expression::operand() const
    {
        return _operand;
    }

    primary_expression& binary_expression::operand()
    {
        return _operand;
    }

    token_position const& binary_expression::position() const
    {
        return _operand.position();
    }

    ostream& operator<<(ostream& os, binary_expression const& expr)
    {
        if (expr.op() == binary_operator::none) {
            return os;
        }
        os << " " << expr.op() << " " << expr.operand();
        return os;
    }

    expression::expression()
    {
    }

    expression::expression(primary_expression first, vector<binary_expression> remainder) :
        _first(std::move(first)),
        _remainder(std::move(remainder))
    {
    }

    primary_expression const& expression::first() const
    {
        return _first;
    }

    primary_expression& expression::first()
    {
        return _first;
    }

    vector<binary_expression> const& expression::remainder() const
    {
        return _remainder;
    }

    vector<binary_expression>& expression::remainder()
    {
        return _remainder;
    }

    token_position const& expression::position() const
    {
        return _first.position();
    }

    bool expression::blank() const
    {
        return _first.blank();
    }

    ostream& operator<<(ostream& os, expression const& expr)
    {
        os << "(" << expr.first();
        for (auto const& bexpr : expr.remainder()) {
            os << bexpr;
        }
        os << ")";
        return os;
    }

}}  // namespace puppet::ast
