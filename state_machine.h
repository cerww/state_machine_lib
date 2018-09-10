#pragma once
#include <tuple>
#include <variant>
#include <array>
#include <algorithm>

template<typename T, T...u>
struct integral_pack {
	using value_type = T;
};

template<typename T, typename ...U>
struct is_in :std::bool_constant<std::disjunction_v<std::is_same<T, U>...>> {

};

template<typename T, typename...U>
static constexpr bool is_in_v = is_in<T, U...>::value;

namespace rawr_m{
template<bool condition>
struct better_conditional {
	template<typename T, typename F>
	using value = T;
};

template<>
struct better_conditional<false> {
	template<typename T, typename F>
	using value = F;
};

template<bool condition, typename T, typename F>
using better_conditional_t = typename better_conditional<condition>::template value<T, F>;
}

template<size_t n, typename T, typename U, typename... rest>
struct type_idx :type_idx<n+1, T, rest...> {

};

template<size_t n, typename T, typename... rest>
struct type_idx<n, T, T, rest...> {
	static constexpr size_t value = n;
};

template<typename T, typename...rest>
static constexpr size_t type_idx_v = type_idx<0, T, rest...>::value;

static_assert(type_idx_v<int, bool, int> == 1);

template<typename T>
struct value_for{
	using value_type = T;
};

template<typename ...Ts>
struct constructible_from{
	template<typename T>
	using type = std::conjunction<std::is_constructible<T, Ts>...>;
};


//stuff


template<typename ...states_c>
struct states{
	static constexpr bool is_moveable = std::conjunction_v<std::is_trivially_move_assignable<states_c>...>;
	using convertable_to_everything = constructible_from<states_c...>;
	//static constexpr bool is_convertable_to_each_other = std::conjunction_v<typename convertable_to_everything::template type<states_c>...>;

	static_assert(is_moveable);
	//static_assert(is_convertable_to_each_other);

	using tuple = std::tuple<states_c...>;
};


template<typename ...events_c>
struct events {
	using tuple = std::tuple<events_c...>;
};

template<typename T, T... things>
struct events_integral {

};


template<typename state,typename... next_states>
struct row{
	using state_t = state;
	using next_state_tuple = std::tuple<next_states...>;

	static constexpr size_t count = sizeof...(next_states);
};

template<typename T>
constexpr bool is_row_f(T a) noexcept{
	return false;
}

template<typename state,typename ...next_states>
constexpr bool is_row_f(row<state,next_states...>) noexcept {
	return true;
}

template<typename T,typename = void>
struct is_row_s:std::false_type{};

template<typename T>
struct is_row_s<T,std::enable_if_t<std::is_default_constructible_v<T>>>//need default constructible cuz i default construct one at compile time
	:std::bool_constant<is_row_f(T{})>{};

template<typename T>
static constexpr bool is_row_v = is_row_s<T>::value;

static_assert(is_row_v<row<int, int>> && is_row_v<row<int, bool>>);
static_assert(!is_row_v<int>);

struct error{};

template<typename state_t, auto next>
struct action_f {};

template<typename state_t, typename next>
struct action_s {};

template<typename T>
struct remove_action {
	using value_type = T;
};

template<typename T, typename F>
struct remove_action<action_s<T, F>> {
	using value_type = T;
};

template<typename T, auto F>
struct remove_action<action_f<T, F>> {
	using value_type = T;
};


template<typename state_type, typename row1, typename...rows>
struct find_correct_row :rawr_m::better_conditional_t<
	std::is_same_v<state_type, typename row1::state_t>, 
	value_for<row1>,
	find_correct_row<state_type, rows...>
> {};



template<typename state_t, typename row>
struct find_correct_row<state_t, row> :value_for<row> {
	static_assert(std::is_same_v<state_t, typename row::state_t>, "failed to find row for state");
};

template<typename next_state, typename prev_state, typename event_t>
constexpr next_state to_next_state(prev_state&& s, event_t&& e) {
	if constexpr(std::is_constructible_v<next_state, prev_state, event_t>) {
		return next_state(std::forward<prev_state>(s), std::forward<event_t>(e));
	}
	else {
		return next_state(std::forward<prev_state>(s));
	}
}

template<typename ...states>
struct state_machine {};



template<typename ... states_c,typename...events_c,typename ...rows_c>
struct state_machine<states<states_c...>,events<events_c...>,rows_c...>{
	static constexpr bool rows_are_valid = std::conjunction_v<is_row_s<rows_c>...>;// (is_row_v<rows_c> && ...);//static_assert doesn't like fold expressions
	static_assert(rows_are_valid);

	template<typename starting_state,std::enable_if_t<is_in_v<std::decay_t<starting_state>,states_c...>,int> = 0>
	constexpr explicit state_machine(starting_state s):m_state(std::in_place_type<std::decay_t<starting_state>>,std::move(s)) {
	}

	template<typename event_t>
	constexpr std::enable_if_t<is_in_v<std::decay_t<event_t>, events_c...>,bool> proccess_event(event_t&& e)noexcept {
		if(m_is_valid)
			std::visit([&](auto& s){
				on_event(s, std::forward<event_t>(e));
			},m_state);
		return m_is_valid;
	}

	constexpr auto& current_state() noexcept{
		return m_state;
	}

	constexpr const auto& current_state()const noexcept{
		return m_state;
	}

	template<typename S, std::enable_if_t<is_in_v<S, states_c...>, int> = 0>
	constexpr bool is_state() const noexcept{
		return std::holds_alternative<S>(m_state);
	}
	constexpr bool is_valid()const noexcept {
		return m_is_valid;
	}
private:
	template<typename state_t,typename event_t>
	constexpr void on_event(state_t& s,event_t&& e) {
		using row = typename find_correct_row<std::decay_t<state_t>, rows_c... >::value_type;		
		static constexpr auto idx = type_idx_v<std::decay_t<event_t>, events_c...>;
		using next_state_t = std::tuple_element_t<idx, typename row::next_state_tuple>;

		if constexpr(std::is_same_v<next_state_t, error>) {
			m_is_valid = false;
		}else{
			m_state.template emplace<next_state_t>(to_next_state<next_state_t>(std::move(s), std::forward<event_t>(e)));
		}
	}
	std::variant<states_c...> m_state;
	bool m_is_valid = true;
};
//integral events ;-;
template<typename ... states_c, typename event_t,event_t... events_c, typename ...rows_c>
struct state_machine<states<states_c...>, events_integral<event_t, events_c...>, rows_c...> {
	static constexpr bool rows_are_valid = std::conjunction_v<is_row_s<rows_c>...>;// (is_row_v<rows_c> && ...);//static_assert doesn't like fold expressions
	static_assert(rows_are_valid);

	template<typename starting_state, std::enable_if_t<is_in_v<std::decay_t<starting_state>, states_c...>, int> = 0>
	constexpr explicit state_machine(starting_state s) :m_state(std::in_place_type<std::decay_t<starting_state>>, std::move(s)) {
	}

	constexpr bool proccess_event(event_t e)noexcept {
		if(m_is_valid)			
			std::visit([&](auto& s) {
				on_event(s, std::forward<event_t>(e));
			}, m_state);
		return m_is_valid;
	}

	constexpr auto& current_state() noexcept {
		return m_state;
	}

	constexpr const auto& current_state()const noexcept {
		return m_state;
	}

	template<typename S,std::enable_if_t<is_in_v<S,states_c...>,int> = 0>
	constexpr bool is_state()const noexcept {
		return std::holds_alternative<S>(m_state);
	}

	constexpr bool is_valid()const noexcept {
		return m_is_valid;
	}

private:

	template<typename current_state_t,typename next_states>
	struct b {};

	template<typename current_state_t,typename ...next_states>
	struct b <current_state_t,std::tuple<next_states...>> {

		template<typename next_state>
		static void to_next_state_impl(current_state_t& c,std::variant<states_c...>& current_state,  event_t e) {
			current_state.template emplace<next_state>(to_next_state<next_state>(std::move(c), std::move(e)));
		}

		static void to_next_state_(current_state_t& c, std::variant<states_c...>& current_state, event_t e,const size_t idx) {
			constexpr static std::array thing { to_next_state_impl<next_states>... };
			thing[idx](c, current_state, std::move(e));
		}
	};

	template<typename state_t>
	constexpr void on_event(state_t& s, event_t e) {
		using row = typename find_correct_row<std::decay_t<state_t>, rows_c... >::value_type;
		using next_state_transformer = b<state_t, typename row::next_state_tuple>;

		const size_t idx = std::distance(s_arr.begin(), std::find(s_arr.begin(), s_arr.end(), e));
		//[[assert: idx!=s_arr.size()]];

		next_state_transformer::to_next_state_(s, m_state, e,idx);
	}

	static constexpr std::array<event_t, sizeof...(events_c)> s_arr{ events_c... };

	std::variant<states_c...> m_state;
	bool m_is_valid = true;
};
//usage:


