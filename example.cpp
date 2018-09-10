#include <state_machine.h>
#include <iostream>
struct base{
	int a;
	int b;
};

struct event1 { int a; };

struct event2 { int b; };

struct state1:base{
	state1() = default;
	state1(base b):base(std::move(b)) {}

	state1(base b,event1 e) :base(std::move(b)) {
		this->a += e.a;
	}
	state1(base b, event2 e) :base(std::move(b)) {
		this->b += e.b;
	}
};


struct state2 :base {
	state2() = default;
	state2(base b) :base(std::move(b)) {}
	state2(base b, event1 e) :base(std::move(b)) {
		this->a += e.a;
	}
	state2(base b, event2 e) :base(std::move(b)) {
		this->b += e.b;
	}
	
};

struct s1 {
	template<typename T>
	s1(T) {};
};
struct s2 {
	template<typename T>
	s2(T) {};
};

inline void asukdhausjdkhasiyd() {
	
	state_machine<
		states<state1, state2>,
		events<		event1, event2>,
		row<state1, state1, state2>,
		row<state2, state2, state1>
	> abc(state1{ {0,0} });

	abc.proccess_event(event1{1});

	std::visit([](auto& a) {
		std::cout << a.a<<std::endl;
		std::cout << typeid(a).name() << std::endl;
	}, abc.current_state());
	abc.proccess_event(event2{3});
	
	std::visit([](auto& a){

		std::cout << a.a << ' '<<a.b<<std::endl;
		std::cout << typeid(a).name() <<std::endl;
	},abc.current_state());
	

	

	state_machine<states<s1, s2>,
		events_integral<char,
				'a', 'b'>,
		row<s1, s1,	 s2>,
		row<s2, s2,	 s1>
	>  ma(s1{1});

	ma.proccess_event('b');


}

#include <string_view>

struct event_1_to_9{
	char n = 0;
	int as_number() const noexcept { return n - '0'; }
};

struct event_anything{};

struct event_zero{};

struct event_plus{};

struct event_minus{};

struct event_decimal{};

struct event_e{};

struct parse_base{	
	int sign = 1;
	double int_part = 0;
	double decimal_part = 0;
	int digits_past_decimal_pt = 0;

	double exp_part = 0;
	int exp_exp = 1;	
};

struct init_state:parse_base {};

struct minus:parse_base{	
	minus(parse_base b):parse_base(std::move(b)) {
		sign = -1;
	}
};

struct zero:parse_base{
	zero(parse_base b) :parse_base(b){}
};

struct done:parse_base{
	done(parse_base b) :parse_base(b){}
	double get() const noexcept{
		return (int_part + decimal_part / std::pow(10, digits_past_decimal_pt))*std::pow(std::pow(10, exp_part), exp_exp)*sign;
	}
};

struct any1:parse_base{
	any1(parse_base b):parse_base(b){}

	any1(parse_base b,event_zero):parse_base(b) {
		int_part *= 10;
	}
	
	any1(parse_base b, event_1_to_9 n) :parse_base(b) {
		int_part *= 10;
		int_part += n.as_number();
	}
};

struct any2:parse_base{
	any2(parse_base b):parse_base(b){}
	any2(parse_base b, event_zero) :parse_base(b) {
		exp_part *= 10;
	}

	any2(parse_base b, event_1_to_9 n) :parse_base(b) {
		exp_part *= 10;
		exp_part += n.as_number();
	}
};

struct exponent:parse_base{
	exponent(parse_base b):parse_base(b){}	
};

struct sign:parse_base{
	sign(parse_base b) :parse_base(b) {}

	sign(parse_base b,event_plus):parse_base(b) {
		exp_exp = 1;
	}

	sign(parse_base b, event_minus) :parse_base(b) {
		exp_exp = -1;
	}
};

struct decimal1 :parse_base {
	decimal1(parse_base b) :parse_base(b) {}
};

struct decimal2 :parse_base {
	decimal2(parse_base b) :parse_base(b) {}

	decimal2(parse_base b,event_1_to_9 n):parse_base(b) {
		decimal_part = decimal_part * 10 + n.as_number();
		++digits_past_decimal_pt;
	}
	decimal2(parse_base b, event_zero) :parse_base(b) {
		decimal_part = decimal_part * 10;
		++digits_past_decimal_pt;
	}
};

inline double to_double(std::string_view str) {
	state_machine<
		states<init_state, minus, zero, done, any1, any2, exponent, sign, decimal1, decimal2>,
		events<			event_zero,	event_1_to_9,	event_e,	event_plus,	event_minus,event_decimal,	event_anything>,
		row<init_state,	zero,		any1,			error,		error,		minus,		error,			error>,
		row<minus,		zero,		any1,			error,		error,		error,		error,			error>,
		row<zero,		done,		done,			exponent,	done,		done,		decimal1,		done>,
		row<any1,		any1,		any1,			exponent,	done,		done,		decimal1,		done>,
		row<decimal1,	decimal2,	decimal2,		error,		error,		error,		error,			error>,
		row<decimal2,	decimal2,	decimal2,		exponent,	done,		done,		done,			done>,
		row<exponent,	any2,		any2,			error,		sign,		sign,		error,			error>,
		row<sign,		any2,		any2,			error,		error,		error,		error,			error>,
		row<any2,		any2,		any2,			done,		done,		done,		done,			done>,
		row<done,		done,		done,			done,		done,		done,		done,			done>
	> sm(init_state{});

	for(int i = 0;i<str.size() && !sm.is_state<done>() && sm.is_valid();++i) {
		switch (str[i]) {
		case '0':
			sm.proccess_event(event_zero{});
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			sm.proccess_event(event_1_to_9{ str[i] });
			break;
		case '-':
			sm.proccess_event(event_minus{});
			break;
		case '+':
			sm.proccess_event(event_plus{});
			break;
		case '.':
			sm.proccess_event(event_decimal{});
			break;
		case 'e':
		case 'E':
			sm.proccess_event(event_e{});
			break;
		default:
			sm.proccess_event(event_anything{});
		}
	}
	if(!sm.is_valid()) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	if(!sm.is_state<done>()) {
		sm.proccess_event(event_anything{});
	}

	return std::get<done>(sm.current_state()).get();

}


int main(){	
	double d = to_double("-123465.45e3");
	std::cout<<d<<std::endl;
}


