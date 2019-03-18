#ifndef BINARYSTREAMS_TRAITS_H
#define BINARYSTREAMS_TRAITS_H
#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <vector>
#include <string>

#include <type_traits>


template <typename Container>
struct is_serializable_container : std::false_type { };

template <typename... Ts> struct is_serializable_container<std::array<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::deque<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::forward_list<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::list<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::set<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::queue<Ts...> > : std::true_type { };
template <typename... Ts> struct is_serializable_container<std::vector<Ts...> > : std::true_type { };

//has push_back
template <typename Container>
struct is_copy_insertable_container : std::false_type { };

template <typename... Ts> struct is_copy_insertable_container<std::deque<Ts...> > : std::true_type { };
template <typename... Ts> struct is_copy_insertable_container<std::list<Ts...> > : std::true_type { };
template <typename... Ts> struct is_copy_insertable_container<std::vector<Ts...> > : std::true_type { };

//has insert
template <typename Container>
struct is_set_container : std::false_type { };

template <typename... Ts> struct is_set_container<std::set<Ts...> > : std::true_type { };

template <typename Container>
struct is_associative_container : std::false_type { };
template <typename... Ts> struct is_associative_container<std::map<Ts...> > : std::true_type { };

//strings
template <typename String>
struct is_string : std::false_type { };
template <typename... Ts> struct is_string<std::basic_string<Ts...> > : std::true_type { };

//
//template <typename Type>
//struct has_serialize : std::false_type { };
//
//template <typename Type, typename = typename Type::serialize> struct has_serialize<Type> : std::true_type { };
#endif
