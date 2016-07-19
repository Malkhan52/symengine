/**
 *  \file polynomial_int.h
 *  Class for Univariate Polynomial Base
 **/
#ifndef SYMENGINE_UINT_BASE_H
#define SYMENGINE_UINT_BASE_H

#include <symengine/basic.h>
#include <symengine/pow.h>
#include <symengine/add.h>
#include <symengine/rational.h>
#include <memory>

#ifdef HAVE_SYMENGINE_FLINT
#include <symengine/flint_wrapper.h>
using fz_t = SymEngine::fmpz_wrapper;
using fq_t = SymEngine::fmpq_wrapper;
#endif
#ifdef HAVE_SYMENGINE_PIRANHA
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#endif

namespace SymEngine
{
// misc methods

#if SYMENGINE_INTEGER_CLASS == SYMENGINE_GMPXX                                 \
    || SYMENGINE_INTEGER_CLASS == SYMENGINE_GMP
#ifdef HAVE_SYMENGINE_FLINT
inline integer_class to_mp_class(const fz_t &i)
{
    integer_class x;
    fmpz_get_mpz(x.get_mpz_t(), i.get_fmpz_t());
    return x;
}
inline rational_class to_mp_class(const fq_t &i)
{
    rational_class x;
    fmpq_get_mpq(x.get_mpq_t(), i.get_fmpq_t());
    return x;
}
#endif

#ifdef HAVE_SYMENGINE_PIRANHA
inline integer_class to_mp_class(const piranha::integer &i)
{
    integer_class x;
    mpz_set(x.get_mpz_t(), i.get_mpz_view());
    return x;
}
inline rational_class to_mp_class(const piranha::rational &i)
{
    rational_class x;
    mpq_set(x.get_mpq_t(), i.get_mpq_view());
    return x;
}
#endif

#elif SYMENGINE_INTEGER_CLASS == SYMENGINE_PIRANHA
#ifdef HAVE_SYMENGINE_FLINT
inline integer_class to_mp_class(const fz_t &i)
{
    integer_class x;
    fmpz_get_mpz(get_mpz_t(x), i.get_fmpz_t());
    return x;
}
inline rational_class to_mp_class(const fq_t &i)
{
    rational_class x;
    fmpq_get_mpq(get_mpq_t(x), i.get_fmpq_t());
    return x;
}
#endif

#elif SYMENGINE_INTEGER_CLASS == SYMENGINE_FLINT
#ifdef HAVE_SYMENGINE_PIRANHA
inline integer_class to_mp_class(const piranha::integer &x)
{
    return integer_class(x.get_mpz_view());
}
inline rational_class to_mp_class(const piranha::rational &x)
{
    return rational_class(x.get_mpq_view());
}
#endif

#endif

inline integer_class to_mp_class(const integer_class &i)
{
    return i;
}

inline rational_class to_mp_class(const rational_class &i)
{
    return i;
}

// dict wrapper
template <typename Key, typename Value, typename Wrapper>
class ODictWrapper
{
public:
    std::map<Key, Value> dict_;

public:
    ODictWrapper() SYMENGINE_NOEXCEPT
    {
    }
    ~ODictWrapper() SYMENGINE_NOEXCEPT
    {
    }

    ODictWrapper(const int &i)
    {
        if (i != 0)
            dict_ = {{0, Value(i)}};
    }

    ODictWrapper(const std::map<Key, Value> &p)
    {
        for (auto &iter : p) {
            if (iter.second != Value(0))
                dict_[iter.first] = iter.second;
        }
    }

    ODictWrapper(const Value &p)
    {
        if (p != Value(0))
            dict_[0] = p;
    }

    ODictWrapper(std::string s)
    {
        dict_[1] = Value(1);
    }

    static Wrapper from_vec(const std::vector<Value> &v)
    {
        Wrapper x;
        x.dict_ = {};
        for (unsigned int i = 0; i < v.size(); i++) {
            if (v[i] != Value(0)) {
                x.dict_[i] = v[i];
            }
        }
        return x;
    }

    Wrapper &operator=(Wrapper &&other) SYMENGINE_NOEXCEPT
    {
        if (this != &other)
            dict_ = std::move(other.dict_);
        return static_cast<Wrapper &>(*this);
    }

    friend Wrapper operator+(const Wrapper &a, const Wrapper &b)
    {
        Wrapper c = a;
        c += b;
        return c;
    }

    Wrapper &operator+=(const Wrapper &other)
    {
        for (auto &iter : other.dict_) {
            auto t = dict_.lower_bound(iter.first);
            if (t != dict_.end() and t->first == iter.first) {
                t->second += iter.second;
                if (t->second == 0) {
                    dict_.erase(t);
                }
            } else {
                dict_.insert(t, {iter.first, iter.second});
            }
        }
        return static_cast<Wrapper &>(*this);
    }

    friend Wrapper operator-(const Wrapper &a, const Wrapper &b)
    {
        Wrapper c = a;
        c -= b;
        return c;
    }

    Wrapper operator-() const
    {
        ODictWrapper c = *this;
        for (auto &iter : c.dict_)
            iter.second *= -1;
        return static_cast<Wrapper &>(c);
    }

    Wrapper &operator-=(const Wrapper &other)
    {
        for (auto &iter : other.dict_) {
            auto t = dict_.lower_bound(iter.first);
            if (t != dict_.end() and t->first == iter.first) {
                t->second -= iter.second;
                if (t->second == 0) {
                    dict_.erase(t);
                }
            } else {
                dict_.insert(t, {iter.first, -iter.second});
            }
        }
        return static_cast<Wrapper &>(*this);
    }

    static Wrapper mul(const Wrapper &a, const Wrapper &b)
    {
        if (a.get_dict().empty())
            return a;
        if (b.get_dict().empty())
            return b;

        Wrapper p;
        for (const auto &i1 : a.dict_)
            for (const auto &i2 : b.dict_)
                p.dict_[i1.first + i2.first] += i1.second * i2.second;

        for (auto it = p.dict_.cbegin(); it != p.dict_.cend();) {
            if (it->second == 0) {
                p.dict_.erase(it++);
            } else {
                ++it;
            }
        }
        return p;
    }

    static Wrapper pow(const Wrapper &a, unsigned int p)
    {
        Wrapper tmp = a, res(1);

        while (p != 1) {
            if (p % 2 == 0) {
                tmp = tmp * tmp;
            } else {
                res = res * tmp;
                tmp = tmp * tmp;
            }
            p >>= 1;
        }

        return (res * tmp);
    }

    friend Wrapper operator*(const Wrapper &a, const Wrapper &b)
    {
        return Wrapper::mul(a, b);
    }

    Wrapper &operator*=(const Wrapper &other)
    {
        if (dict_.empty())
            return static_cast<Wrapper &>(*this);

        if (other.dict_.empty()) {
            dict_.clear();
            return static_cast<Wrapper &>(*this);
        }

        // ! other is a just constant term
        if (other.dict_.size() == 1
            and other.dict_.find(0) != other.dict_.end()) {
            auto t = other.dict_.begin();
            for (auto &i1 : dict_)
                i1.second *= t->second;
            return static_cast<Wrapper &>(*this);
        }

        Wrapper res = Wrapper::mul(static_cast<Wrapper &>(*this), other);
        res.dict_.swap(this->dict_);
        return static_cast<Wrapper &>(*this);
    }

    bool operator==(const Wrapper &other) const
    {
        return dict_ == other.dict_;
    }

    bool operator!=(const Wrapper &other) const
    {
        return not(*this == other);
    }

    const std::map<Key, Value> &get_dict() const
    {
        return dict_;
    }

    unsigned int size() const
    {
        return dict_.size();
    }

    bool empty() const
    {
        return dict_.empty();
    }

    Key degree() const
    {
        if (dict_.empty())
            return Key(0);
        return dict_.rbegin()->first;
    }

    Value get_coeff(Key x) const
    {
        auto ite = dict_.find(x);
        if (ite != dict_.end())
            return ite->second;
        return Value(0);
    }

    Value get_lc() const
    {
        if (dict_.empty())
            return Value(0);
        return dict_.rbegin()->second;
    }
};

umap_basic_num _find_gens_poly(const RCP<const Basic> &x);

template <typename Container, typename Poly>
class UPolyBase : public Basic
{
protected:
    RCP<const Basic> var_;
    Container poly_;

public:
    UPolyBase(const RCP<const Basic> &var, Container &&container)
        : var_{var}, poly_{container}
    {
    }

    typedef Container container_type;

    //! \returns `-1`,`0` or `1` after comparing
    virtual int compare(const Basic &o) const = 0;
    virtual std::size_t __hash__() const = 0;

    //! \returns `true` if two objects are equal
    inline bool __eq__(const Basic &o) const
    {
        if (is_a<Poly>(o))
            return eq(*var_, *(static_cast<const Poly &>(o).var_))
                   and poly_ == static_cast<const Poly &>(o).poly_;
        return false;
    }

    inline const RCP<const Basic> &get_var() const
    {
        return var_;
    }

    inline const Container &get_poly() const
    {
        return poly_;
    }

    inline vec_basic get_args() const
    {
        return {};
    }

    static RCP<const Poly> from_container(const RCP<const Basic> &var,
                                          Container &&d)
    {
        return make_rcp<const Poly>(var, std::move(d));
    }
};

// super class for all non-expr polys, all methods which are
// common for all non-expr polys go here eg. degree, eval etc.
template <typename Cont, typename P, typename C>
class UNonExprPoly : public UPolyBase<Cont, P>
{
public:
    typedef C coef_type;

    UNonExprPoly(const RCP<const Basic> &var, Cont &&container)
        : UPolyBase<Cont, P>(var, std::move(container))
    {
    }

    // return coefficient of degree 'i'
    virtual C get_coeff(unsigned int i) const = 0;
    // return value of poly when ealudated at `x`
    virtual C eval(const C &x) const = 0;
    virtual std::vector<C> multieval(const std::vector<C> &x) const = 0;
    // return `degree` + 1. `0` returned for zero poly.
    virtual unsigned int size() const = 0;

    inline unsigned int get_degree() const
    {
        return this->poly_.degree();
    }

    C get_lc() const
    {
        return get_coeff(get_degree());
    }

    static RCP<const P> from_dict(const RCP<const Basic> &var,
                                  std::map<unsigned, C> &&d)
    {
        return P::from_container(var, P::cont_from_dict(var, std::move(d)));
    }
};

template <typename D, typename P>
class UIntPolyBase : public UNonExprPoly<D, P, integer_class>
{
public:
    UIntPolyBase(const RCP<const Basic> &var, D &&container)
        : UNonExprPoly<D, P, integer_class>(var, std::move(container))
    {
    }

    RCP<const Basic> as_symbolic() const
    {
        auto it = (static_cast<const P &>(*this)).begin();
        auto end = (static_cast<const P &>(*this)).end();

        vec_basic args;
        for (; it != end; ++it) {
            integer_class m = it->second;

            if (it->first == 0) {
                args.push_back(integer(m));
            } else if (it->first == 1) {
                if (m == 1) {
                    args.push_back(this->var_);
                } else {
                    args.push_back(
                        Mul::from_dict(integer(m), {{this->var_, one}}));
                }
            } else {
                if (m == 1) {
                    args.push_back(pow(this->var_, integer(it->first)));
                } else {
                    args.push_back(Mul::from_dict(
                        integer(m), {{this->var_, integer(it->first)}}));
                }
            }
        }
        return SymEngine::add(args);
    }
};

template <typename D, typename P>
class URatPolyBase : public UNonExprPoly<D, P, rational_class>
{
public:
    URatPolyBase(const RCP<const Basic> &var, D &&container)
        : UNonExprPoly<D, P, rational_class>(var, std::move(container))
    {
    }

    RCP<const Basic> as_symbolic() const
    {
        auto it = (static_cast<const P &>(*this)).begin();
        auto end = (static_cast<const P &>(*this)).end();

        vec_basic args;
        for (; it != end; ++it) {
            rational_class m = it->second;

            if (it->first == 0) {
                args.push_back(Rational::from_mpq(m));
            } else if (it->first == 1) {
                if (m == 1) {
                    args.push_back(this->var_);
                } else {
                    args.push_back(Mul::from_dict(Rational::from_mpq(m),
                                                  {{this->var_, one}}));
                }
            } else {
                if (m == 1) {
                    args.push_back(pow(this->var_, integer(it->first)));
                } else {
                    args.push_back(
                        Mul::from_dict(Rational::from_mpq(m),
                                       {{this->var_, integer(it->first)}}));
                }
            }
        }
        return SymEngine::add(args);
    }
};

template <typename T, typename Int>
class ContainerBaseIter
{
protected:
    RCP<const T> ptr_;
    long i_;

public:
    ContainerBaseIter(RCP<const T> ptr, long x) : ptr_{ptr}, i_{x}
    {
    }

    bool operator==(const ContainerBaseIter &rhs)
    {
        return (ptr_ == rhs.ptr_) and (i_ == rhs.i_);
    }

    bool operator!=(const ContainerBaseIter &rhs)
    {
        return not(*this == rhs);
    }

    std::pair<long, Int> operator*()
    {
        return std::make_pair(i_, ptr_->get_coeff_ref(i_));
    }

    std::shared_ptr<std::pair<long, Int>> operator->()
    {
        return std::make_shared<std::pair<long, Int>>(i_,
                                                      ptr_->get_coeff_ref(i_));
    }
};

template <typename T, typename Int>
class ContainerForIter : public ContainerBaseIter<T, Int>
{
public:
    ContainerForIter(RCP<const T> ptr, long x)
        : ContainerBaseIter<T, Int>(ptr, x)
    {
    }

    ContainerForIter operator++()
    {
        this->i_++;
        while (this->i_ < this->ptr_->size()) {
            if (this->ptr_->get_coeff_ref(this->i_) != 0)
                break;
            this->i_++;
        }
        return *this;
    }
};

template <typename T, typename Int>
class ContainerRevIter : public ContainerBaseIter<T, Int>
{
public:
    ContainerRevIter(RCP<const T> ptr, long x)
        : ContainerBaseIter<T, Int>(ptr, x)
    {
    }

    ContainerRevIter operator++()
    {
        this->i_--;
        while (this->i_ >= 0) {
            if (this->ptr_->get_coeff_ref(this->i_) != 0)
                break;
            this->i_--;
        }
        return *this;
    }
};

template <typename Poly>
RCP<const Poly> add_upoly(const Poly &a, const Poly &b)
{
    if (!(a.get_var()->__eq__(*b.get_var())))
        throw std::runtime_error("Error: variables must agree.");

    auto dict = a.get_poly();
    dict += b.get_poly();
    return Poly::from_container(a.get_var(), std::move(dict));
}

template <typename Poly>
RCP<const Poly> neg_upoly(const Poly &a)
{
    auto dict = a.get_poly();
    dict = -dict;
    return Poly::from_container(a.get_var(), std::move(dict));
}

template <typename Poly>
RCP<const Poly> sub_upoly(const Poly &a, const Poly &b)
{
    if (!(a.get_var()->__eq__(*b.get_var())))
        throw std::runtime_error("Error: variables must agree.");

    auto dict = a.get_poly();
    dict -= b.get_poly();
    return Poly::from_container(a.get_var(), std::move(dict));
}

template <typename Poly>
RCP<const Poly> mul_upoly(const Poly &a, const Poly &b)
{
    if (!(a.get_var()->__eq__(*b.get_var())))
        throw std::runtime_error("Error: variables must agree.");

    auto dict = a.get_poly();
    dict *= b.get_poly();
    return Poly::from_container(a.get_var(), std::move(dict));
}

template <typename Poly>
RCP<const Poly> pow_upoly(const Poly &a, unsigned int p)
{
    auto dict = Poly::container_type::pow(a.get_poly(), p);
    return Poly::from_container(a.get_var(), std::move(dict));
}

template <typename Poly>
RCP<const Poly> quo_upoly(const Poly &a, const Poly &b)
{
    if (!(a.get_var()->__eq__(*b.get_var())))
        throw std::runtime_error("Error: variables must agree.");

    auto dict = a.get_poly();
    dict /= b.get_poly();
    return Poly::from_dict(a.get_var(), std::move(dict));
}
}

#endif // SYMENGINE_UINT_BASE_H
