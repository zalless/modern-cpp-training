#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

template <typename T, typename = void>
struct is_container : std::false_type {};

template <typename T>
struct is_container<T,
                    std::void_t<decltype(std::declval<T>().begin()),
                                decltype(std::declval<T>().end()),
                                typename T::iterator>> : std::true_type {};

template <typename T, typename = void>
struct is_pointer_like : std::false_type {};

template <typename T>
struct is_pointer_like<T,
                       std::void_t<decltype(std::declval<T>().operator->()),
                                   decltype(std::declval<T>().operator*())>>
    : std::true_type {};

template <typename T, class Writer, typename = void>
struct has_member_function_serialize : std::false_type {};

template <typename T, class Writer>
struct has_member_function_serialize<
    T,
    Writer,
    std::void_t<decltype(std::declval<T>().serialize(std::declval<Writer>()))>>
    : std::true_type {};


template <typename T>
void print(std::ostream& os, const T& v, std::string_view n = {});

template <typename T>
void
print_default(std::ostream& os, const T& v)
{
    if constexpr (std::is_convertible_v<T, std::string_view>)
    {
        os << '\"' << v << '\"';
    }
    else
    {
        os << v;
    }
}

void
print_default(std::ostream& os, bool v)
{
    os << std::boolalpha << v << std::noboolalpha;
}

template <typename T>
void
print_container(std::ostream& os, const T& v)
{
    std::cout << '[';
    for (const auto& x : v)
    {
        print(os, x, {});
        std::cout << ",";
    }
    std::cout << '\b' << ']';
}

template <typename T>
auto
makeNullValue()
{
    if constexpr (std::is_assignable_v<T, std::nullopt_t>)
    {
        return std::nullopt;
    }
    else
    {
        return nullptr;
    }
}

template <typename T>
void
print_pointer(std::ostream& os, const T& v)
{
    if (v == makeNullValue<T>())
    {
        std::cout << "null";
    }
    else
    {
        print(os, *v, {});
    }
}

template <typename T>
void print(std::ostream& os, const T& v, std::string_view n);

template <typename T>
using Nvp = std::pair<std::string_view, const T&>;

template <typename T>
Nvp<T>
makeNvp(std::string_view n, const T& v)
{
    return std::make_pair(n, std::cref(v));
}

class Writer
{
    std::ostream& mOs;

public:
    Writer(std::ostream& os) : mOs(os) {}

    template <typename T>
    friend Writer&
    operator<<(Writer& writer, Nvp<T> nvp)
    {
        print(writer.mOs, nvp.second, nvp.first);
        return writer;
    }

    template <typename T>
    friend Writer&
    operator<<(Writer& writer, const T& v)
    {
        print(writer.mOs, v);
        return writer;
    }
};

template <typename T>
void
print(std::ostream& os, const T& v, std::string_view n)
{
    if (!n.empty())
    {
        std::cout << '\"' << n << '\"' << ':';
    }

    if constexpr (is_container<T>::value && !std::is_convertible_v<T, std::string>)
    {
        print_container(os, v);
    }
    else if constexpr (is_pointer_like<T>::value || std::is_pointer_v<T>)
    {
        print_pointer(os, v);
    }
    else if constexpr (has_member_function_serialize<T, Writer&>::value)
    {
        Writer writer{os};
        writer << '{';
        v.serialize(writer);
        writer << '}';
    }
    else
    {
        print_default(os, v);
    }
}

struct Foo
{
    int i;
    std::string s;

    template <class Writer>
    void
    serialize(Writer& writer) const
    {
        writer << makeNvp("i", i) << ',' << makeNvp("s", s);
    }
};

struct Bar
{
    std::vector<Foo> foos;

    template <class Writer>
    void
    serialize(Writer& writer) const
    {
        writer << makeNvp("foos", foos);
    }
};

struct Baz
{
    std::string name;
    bool valid;
    std::optional<Bar> bar;

    template <class Writer>
    void
    serialize(Writer& writer) const
    {
        writer << makeNvp("name", name) << ',' << makeNvp("valid", valid) << ','
               << makeNvp("bar", bar);
    }
};

struct A
{
};

int
main()
{
    Writer writer(std::cout);

    writer << makeNvp("x", 123) << '\n';
    writer << makeNvp("x", 0.4) << '\n';
    writer << makeNvp("x", "Print me") << '\n';

    std::vector<int> v = {1, 2, 3};
    writer << makeNvp("numbers", v) << '\n';

    int x = 5;
    writer << makeNvp("x", &x) << '\n';
    int* y = nullptr;
    writer << makeNvp("y", &y) << '\n';

    auto uptr = std::make_unique<int>(543);
    writer << makeNvp("uptr", uptr) << '\n';

    auto sptr = std::make_shared<int>(543);
    writer << makeNvp("sptr", sptr) << '\n';

    auto opt = std::make_optional<int>(65);
    writer << makeNvp("opt", opt) << '\n';

    std::vector<std::optional<int>> vo = {4, 5, 6, std::nullopt, 8};
    writer << makeNvp("vector_of_optionals", vo) << '\n';

    std::optional<std::vector<std::optional<int>>> ovo = {{4, 5, 6, std::nullopt, 8}};
    writer << makeNvp("optioal_vector_of_optionals", ovo) << '\n';

    Foo foo{20, "foo"};
    writer << makeNvp("foo", foo) << '\n';

    Bar bar{{Foo{10, "foo10"}, Foo{20, "foo20"}}};
    Baz baz{"Some cool BAZ", true, bar};
    writer << baz << '\n';
}
