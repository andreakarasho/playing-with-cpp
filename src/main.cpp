#include <iostream>
#include <tuple>
#include <vector>
#include <type_traits>

// Example components
struct Position
{
	float x, y;
};
struct Velocity
{
	float dx, dy;
};
struct Npc
{
};
struct Health
{
	int hp;
};

// Entity struct combining components
struct Entity
{
	std::tuple<Position, Velocity, Npc, Health> components;
};

// Filter structs
template <typename T>
struct With
{
	using type = T;
};

template <typename T>
struct Without
{
	using type = T;
};

// Type trait to check if a type is in a tuple
template <typename T, typename Tuple>
struct is_in_tuple;

template <typename T, typename... Us>
struct is_in_tuple<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...>
{
};

// Check if a type is a tuple
template <typename T>
struct is_tuple : std::false_type
{
};

template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type
{
};

// Helper function to check if an entity matches filters
template <typename... Filters, typename Components>
bool matches_filters(const Components& components)
{
	return (matches_filter<Filters>(components) && ...);
}

template <typename Filter, typename Components>
bool matches_filter(const Components& components)
{
	if constexpr (std::is_same_v<Filter, With<typename Filter::type>>)
	{
		return is_in_tuple<typename Filter::type, Components>::value;
	}
	else if constexpr (std::is_same_v<Filter, Without<typename Filter::type>>)
	{
		return !is_in_tuple<typename Filter::type, Components>::value;
	}
	return true;
}

// Helper function to get a component from a tuple
template <typename T, typename Tuple>
T& get_component(Tuple& components)
{
	return std::get<T>(components);
}

// Specialization for tuples
template <typename Tuple, typename Components, std::size_t... Is>
auto get_components_impl(Components& components, std::index_sequence<Is...>)
{
	return std::tie(get_component<std::tuple_element_t<Is, Tuple>>(components)...);
}

template <typename Tuple, typename Components>
auto get_components(Components& components)
{
	return get_components_impl<Tuple>(components, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

// Query class template
template <typename TData, typename TFilter>
class Query
{
public:
	Query(std::vector<Entity>& entities) : entities(entities) {}

	template <typename Lambda>
	void execute(Lambda lambda)
	{
		for (auto& entity : entities)
		{
			if (matches_filters_impl<TFilter>(entity.components))
			{
				if constexpr (is_tuple<TData>::value)
				{
					std::apply(lambda, get_components<TData>(entity.components));
				}
				else
				{
					lambda(get_component<TData>(entity.components));
				}
			}
		}
	}

private:
	std::vector<Entity>& entities;

	// Check if an entity matches all filters
	template <typename Filters, typename Components>
	bool matches_filters_impl(const Components& components)
	{
		if constexpr (is_tuple<Filters>::value)
		{
			return matches_filters_from_tuple<Filters>(components);
		}
		else
		{
			return matches_filter<Filters>(components);
		}
	}

	template <typename Tuple, typename Components, std::size_t... Is>
	bool matches_filters_from_tuple_impl(const Components& components, std::index_sequence<Is...>)
	{
		return (matches_filter<std::tuple_element_t<Is, Tuple>>(components) && ...);
	}

	template <typename Tuple, typename Components>
	bool matches_filters_from_tuple(const Components& components)
	{
		return matches_filters_from_tuple_impl<Tuple>(components, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
	}
};

int main()
{
	// Define some entities
	Entity e1 = { std::make_tuple(Position{1, 2}, Velocity{0.1f, 0.2f}, Npc{}, Health{100}) };
	Entity e2 = { std::make_tuple(Position{3, 4}, Velocity{0.3f, 0.4f}, Npc{}, Health{80}) };
	Entity e3 = { std::make_tuple(Position{5, 6}, Velocity{0.5f, 0.6f}, Npc{}, Health{60}) };

	std::vector<Entity> entities = { e1, e2, e3 };

	// Query with filter: With<Position> and Without<Npc>
	using TData = std::tuple<Position, Velocity>;
	using TFilter = std::tuple<With<Position>>;

	Query<TData, TFilter> query(entities);

	query.execute([](auto& pos, auto& vel)
		{
			std::cout << "Position: {" << pos.x << ", " << pos.y << "}, ";
			std::cout << "Velocity: {" << vel.dx << ", " << vel.dy << "}\n";
		});

	return 0;
}
