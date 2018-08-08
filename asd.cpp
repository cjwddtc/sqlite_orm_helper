#include "sqlite_orm.h"
#include <functional>
#include <tuple>
#if __cplusplus > 201402L
using std::apply;

#else
namespace detail
{
    template < class F, class Tuple, std::size_t... I >
    constexpr decltype(auto) apply_impl(F&& f, Tuple&& t,
                                        std::index_sequence< I... >)
    {
        return std::invoke(std::forward< F >(f),
                           std::get< I >(std::forward< Tuple >(t))...);
    }
}

template < class F, class Tuple >
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
    return detail::apply_impl(
        std::forward< F >(f), std::forward< Tuple >(t),
        std::make_index_sequence<
            std::tuple_size_v< std::remove_reference_t< Tuple > > >{});
}
#endif
template < class T >
struct asd
{
    typedef std::function< void(T) > value_type;
    value_type                       a;
    asd(value_type b)
        : a(b)
    {
    }
    void operator()(T& a) {}
};

template < class... ARG >
struct asd< std::tuple< ARG... > >
{
    typedef std::function< void(ARG&...) > value_type;
    value_type                             a;
    void                                   operator()(std::tuple< ARG... >& b)
    {
        apply(a, b);
    }
    asd(value_type b)
        : a(b)
    {
    }
};

template < typename T >
struct memfun_type
{
    using type = void;
};
template < typename Ret, typename Class, typename... Args >
struct memfun_type< Ret (Class::*)(Args...) const >
{
    using type = std::function< Ret(Args...) >;
};
template < typename F >
typename memfun_type< decltype(&F::operator()) >::type FFL(F const& func)
{ // Function from lambda!
    return func;
}


template < class DB_TYPE, class T, class... Args,
           class Ret
           = typename sqlite_orm::internal::column_result_t< T >::type >
void transform(size_t max_count, DB_TYPE& db, T m,
               typename asd< Ret >::value_type f, Args&&... args)
{
    size_t     i = 0;
    asd< Ret > b = f;
    while (true)
    {

        std::vector< Ret > buf = db.select(m, std::forward< Args&& >(args)...,
                                           sqlite_orm::limit(i, max_count));
        for (auto& a : buf)
        {
            b(a);
        }
        if (buf.size() != max_count)
        {
            break;
        }
        else
        {
            i += max_count;
        }
    }
}

template < class DB_TYPE, class T, class... Args >
void transform(size_t max_count, DB_TYPE& db, std::function< void(T&) > s,
               Args&&... args)
{
    size_t i = 0;
    while (true)
    {

        std::vector< T > buf = db.get_all< T >(std::forward< Args&& >(args)...,
                                               sqlite_orm::limit(i, max_count));
        for (auto& a : buf)
        {
            s(a);
        }
        if (buf.size() != max_count)
        {
            break;
        }
        else
        {
            i += max_count;
        }
    }
}

template < class DB_TYPE, class F, class... Args >
void transform(size_t max_count, DB_TYPE& db, F s, Args&&... args,
               decltype(FFL(s)) = {})
{
    transform(max_count, db, FFL(s), std::forward< Args&& >(args)...);
}

template < class DB_TYPE >
struct ddb
{
    DB_TYPE db;
    ddb(DB_TYPE d)
        : db(d)
    {
    }
    template < class ...ARG>
	void transform(ARG&&... arg)
	{
        ::transform(10, db, std::forward< ARG&& >(arg)...);
	}
};

struct Employee
{
    int                            id;
    std::string                    name;
    int                            age;
    std::shared_ptr< std::string > address; //  optional
    std::shared_ptr< double >      salary;  //  optional
};

using namespace sqlite_orm;
#include <iostream>
using std::cout;
using std::endl;

int main(int argc, char** argv)
{
    auto storage = make_storage(
        "select.sqlite",
        make_table("COMPANY", make_column("ID", &Employee::id, primary_key()),
                   make_column("NAME", &Employee::name),
                   make_column("AGE", &Employee::age),
                   make_column("ADDRESS", &Employee::address),
                   make_column("SALARY", &Employee::salary)));
    storage.sync_schema();
    storage.remove_all< Employee >(); //  remove all old employees in case they
                                      //  exist in db..

    //  create employees..
    Employee paul{-1, "Paul", 32, std::make_shared< std::string >("California"),
                  std::make_shared< double >(20000.0)};
    Employee allen{-1, "Allen", 25, std::make_shared< std::string >("Texas"),
                   std::make_shared< double >(15000.0)};
    Employee teddy{-1, "Teddy", 23, std::make_shared< std::string >("Norway"),
                   std::make_shared< double >(20000.0)};
    Employee mark{-1, "Mark", 25, std::make_shared< std::string >("Rich-Mond"),
                  std::make_shared< double >(65000.0)};
    Employee david{-1, "David", 27, std::make_shared< std::string >("Texas"),
                   std::make_shared< double >(85000.0)};
    Employee kim{-1, "Kim", 22, std::make_shared< std::string >("South-Hall"),
                 std::make_shared< double >(45000.0)};
    Employee james{-1, "James", 24, std::make_shared< std::string >("Houston"),
                   std::make_shared< double >(10000.0)};

    //  insert employees. `insert` function returns id of inserted object..
    paul.id  = storage.insert(paul);
    allen.id = storage.insert(allen);
    teddy.id = storage.insert(teddy);
    mark.id  = storage.insert(mark);
    david.id = storage.insert(david);
    kim.id   = storage.insert(kim);
    james.id = storage.insert(james);

    //  print users..
    cout << "paul = " << storage.dump(paul) << endl;
    cout << "allen = " << storage.dump(allen) << endl;
    cout << "teddy = " << storage.dump(teddy) << endl;
    cout << "mark = " << storage.dump(mark) << endl;
    cout << "david = " << storage.dump(david) << endl;
    cout << "kim = " << storage.dump(kim) << endl;
    cout << "james = " << storage.dump(james) << endl;

    //  select all employees..
    auto allEmployees = storage.get_all< Employee >();

    cout << "allEmployees[0] = " << storage.dump(allEmployees[0]) << endl;
    cout << "allEmployees count = " << allEmployees.size() << endl;
    storage.select(columns(&Employee::id, &Employee::name, &Employee::age),
                   sqlite_orm::limit(0, 1000));
    ddb<decltype(storage)> a = storage;
    a.transform([](Employee& a) { std::cout << a.address << std::endl; });
    a.transform(columns(&Employee::id, &Employee::name, &Employee::age),
                [](auto a,auto b,auto c) {
                    std::cout << a << b << c << std::endl;
                });
}