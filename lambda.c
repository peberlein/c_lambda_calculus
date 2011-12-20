/* lambda calculus with church encoding in C
   Pete Eberlein <pete.eberlein@gmail.com>

   inspired by http://experthuman.com/programming-with-nothing 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gc/gc.h>

typedef struct functor *F;

struct functor {
	F (*call)(F, F);
	union {
		F (*call_0)(F);
		F (*call_1)(F, F);
		F (*call_2)(F, F, F);
		F (*call_3)(F, F, F, F);
		F (*call_4)(F, F, F, F, F);
		F (*call_5)(F, F, F, F, F, F);
		F (*call_6)(F, F, F, F, F, F, F);
		F (*call_7)(F, F, F, F, F, F, F, F);
		F (*call_8)(F, F, F, F, F, F, F, F, F);
	};
	F a[0];
};

F dispatch0(F p, F x) { return p->call_0(x); }
F dispatch1(F p, F x) { return p->call_1(x,p->a[0]); }
F dispatch2(F p, F x) { return p->call_2(x,p->a[0],p->a[1]); }
F dispatch3(F p, F x) { return p->call_3(x,p->a[0],p->a[1],p->a[2]); }
F dispatch4(F p, F x) { return p->call_4(x,p->a[0],p->a[1],p->a[2],p->a[3]); }
F dispatch5(F p, F x) { return p->call_5(x,p->a[0],p->a[1],p->a[2],p->a[3],p->a[4]); }
F dispatch6(F p, F x) { return p->call_6(x,p->a[0],p->a[1],p->a[2],p->a[3],p->a[4],p->a[5]); }
F dispatch7(F p, F x) { return p->call_7(x,p->a[0],p->a[1],p->a[2],p->a[3],p->a[4],p->a[5],p->a[6]); }
F dispatch8(F p, F x) { return p->call_8(x,p->a[0],p->a[1],p->a[2],p->a[3],p->a[4],p->a[5],p->a[6],p->a[7]); }

F (*lambda_dispatch[])(F, F) = {
	dispatch0,
	dispatch1,
	dispatch2,
	dispatch3,
	dispatch4,
	dispatch5,
	dispatch6,
	dispatch7,
	dispatch8,
};

/* Create a lambda function with one argument, an expression, 
   and zero-or-more captured variables.  Callable with call function.
   Example:
   	LAMBDA(x,
		LAMBDA(y,
			CALL(y, x),
		x)
	)
   Note: variable captures are limited to 8, due to lambda_dispatch size.
*/
#define LAMBDA(__arg, __expr, ...) ({	\
	F __f (__arg, ##__VA_ARGS__) F __arg, ##__VA_ARGS__; { return __expr; } \
	struct {struct functor __func; F __a[0], ##__VA_ARGS__; } *__this;	\
	__this = GC_MALLOC(sizeof(*__this));	\
	*__this = (typeof(*__this)){ 	\
		{lambda_dispatch[(sizeof(*__this)-sizeof(__this->__func))/sizeof(F)], __f},	\
		{}, ##__VA_ARGS__};	\
	(F)__this;	\
	})
	
/* call arg terminator */
F _ = (F)-1;

/* call f with args sequentially until _ */
#define CALL(f, ...) call(f, __VA_ARGS__, _)
F call(F f, ...)
{
	va_list ap;
	F a;
	va_start(ap, f);
	for (a = va_arg(ap, F); a != _; a = va_arg(ap, F))
		f = f->call(f, a);
	va_end(ap);
	return f;
}

long to_long(F f)
{
	F inner(F p, F x)
	{
		//printf("to_long inner p=%p x=%p\n", p, x);
		return (F)((long)x + 1);
	}
	struct functor lambda = {inner};
	//printf("to_long lambda=%p inner=%p call=%p\n", &lambda, inner, lambda.call);
	return (long) CALL(f, &lambda, 0);
}

const char *to_boolean(F f)
{
	return (const char *) CALL(f, (F)"true", (F)"false");
}


#define ZERO		\
LAMBDA(p,		\
	LAMBDA(x, x)	\
)

#define P(x) (p->call(p, x))

#define ONE		\
LAMBDA(p, 		\
	LAMBDA(x,	\
		P(x),	\
	p)		\
)

#define TWO			\
LAMBDA(p, 			\
	LAMBDA(x,		\
		P(P(x)),	\
	p)			\
)

#define THREE			\
LAMBDA(p,			\
	LAMBDA(x,		\
		P(P(P(x))),	\
	p)			\
)

#define FIVE				\
LAMBDA(p,				\
	LAMBDA(x,			\
		P(P(P(P(P(x))))),	\
	p)				\
)

#define FIFTEEN				\
LAMBDA(p,				\
	LAMBDA(x,			\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(x)))))	\
		)))))))))),		\
	p)				\
)

#define HUNDRED				\
LAMBDA(p,				\
	LAMBDA(x,			\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(	\
		P(P(P(P(P(P(P(P(P(P(x	\
		))))))))))))))))))))	\
		))))))))))))))))))))	\
		))))))))))))))))))))	\
		))))))))))))))))))))	\
		)))))))))))))))))))),	\
	p)				\
)

#define FALSE		\
LAMBDA(x,		\
	LAMBDA(y, y)	\
)

#define TRUE		\
LAMBDA(x,		\
	LAMBDA(y,	\
		x,	\
	x)		\
)

/* if function before simplification */
#define IF_0					\
LAMBDA(b,					\
	LAMBDA(x,			\
		LAMBDA(y,		\
			CALL(b, x, y),	\
		b, x),			\
	b)					\
)

#define IF		\
LAMBDA(b, b)

#define IS_ZERO				\
LAMBDA(n,				\
	CALL(n, LAMBDA(x, FALSE), TRUE)	\
)

#define INC						\
LAMBDA(n,						\
	LAMBDA(p,				\
		LAMBDA(x,			\
			CALL(p, CALL(n, p, x)),	\
		n, p),				\
	n)						\
)

#define DEC							\
LAMBDA(n,							\
	LAMBDA(f,					\
		LAMBDA(x,				\
			CALL(n,				\
				LAMBDA(g,			\
					LAMBDA(h,	\
						CALL(h, CALL(g, f)), \
					f, g),		\
				f),				\
				LAMBDA(y,		\
					x,		\
				x),				\
				LAMBDA(y,			\
					y		\
				)),				\
		n, f),					\
	n)							\
)

#define ADD				\
LAMBDA(m,				\
	LAMBDA(n,			\
		CALL(n, INC, m),	\
	m)				\
)

#define SUB				\
LAMBDA(m,				\
	LAMBDA(n,			\
		CALL(n, DEC, m),	\
	m)				\
)

#define MUL					\
LAMBDA(m,					\
	LAMBDA(n,				\
		CALL(n, CALL(ADD, m), ZERO),	\
	m)					\
)

#define POW					\
LAMBDA(m,					\
	LAMBDA(n,				\
		CALL(n, CALL(MUL, m), ONE),	\
	m)					\
)

#define IS_LESS_OR_EQUAL			\
LAMBDA(m,					\
	LAMBDA(n,				\
		CALL(IS_ZERO, CALL(SUB, m, n)),	\
	m)					\
)

/* the Y combinator */
#define Y				\
LAMBDA(f,				\
	CALL(LAMBDA(x,			\
		CALL(f, CALL(x, x)),	\
	f), LAMBDA(x,			\
		CALL(f, CALL(x, x)),	\
	f))				\
)

/* the Z combinator */
#define Z				\
LAMBDA(f,				\
	CALL(LAMBDA(x,			\
		CALL(f, LAMBDA(y,	\
			CALL(x, x, y),	\
		x)),			\
	f), LAMBDA(x,			\
		CALL(f, LAMBDA(y,	\
			CALL(x, x, y),	\
		x)),			\
	f))				\
)

#define MOD 					\
CALL(Z, LAMBDA(f,				\
	LAMBDA(m,			\
		LAMBDA(n,		\
			CALL(IF, CALL(IS_LESS_OR_EQUAL, n, m),	\
				LAMBDA(x,				\
					CALL(f, CALL(SUB, m, n), n, x),	\
				f, m, n),				\
				m),		\
		f, m),			\
	f)					\
))

#define PAIR 				\
LAMBDA(x,				\
	LAMBDA(y,			\
		LAMBDA(f,		\
			CALL(f, x, y),	\
		x, y),			\
	x)				\
)

#define LEFT 				\
LAMBDA(p,				\
	CALL(p, LAMBDA(x,		\
		LAMBDA(y,		\
			x,		\
		x)			\
	))				\
)

#define RIGHT					\
LAMBDA(p,					\
	CALL(p, LAMBDA(x,		\
		LAMBDA(y,		\
			y		\
		)				\
	))					\
)

#define UNSHIFT					\
LAMBDA(l,					\
	LAMBDA(x,				\
		CALL(PAIR, FALSE,		\
			CALL(PAIR, x, l)),	\
	l)					\
)

#define EMPTY	CALL(PAIR, TRUE, TRUE)
#define IS_EMPTY LEFT
#define FIRST	LAMBDA(l, CALL(LEFT, CALL(RIGHT, l)))
#define REST	LAMBDA(l, CALL(RIGHT, CALL(RIGHT, l)))

void print_list(F l)
{
	while (CALL(IF, CALL(IS_EMPTY, l), (F)0, (F)1)) {
		printf("%ld ", to_long(CALL(FIRST, l)));
		l = CALL(REST, l);
	}
	printf("\n");
}

#define RANGE					\
CALL(Z, LAMBDA(f,				\
	LAMBDA(m,			\
		LAMBDA(n,		\
			CALL(IF, CALL(IS_LESS_OR_EQUAL, m, n),	\
				LAMBDA(x,				\
					CALL(UNSHIFT, CALL(f, CALL(INC, m), n), m, x),\
				f, m, n),	\
				EMPTY),		\
		f, m),			\
	f)					\
))

#define FOLD					\
CALL(Z, LAMBDA(f,				\
	LAMBDA(l,			\
		LAMBDA(x,		\
			LAMBDA(g,	\
				CALL(IF, CALL(IS_EMPTY, l),	\
					x,				\
					LAMBDA(y,			\
						CALL(g, CALL(f, CALL(REST, l), x, g), CALL(FIRST, l), y), \
					f, l, x, g)),		\
			f, l, x),				\
		f, l),			\
	f)					\
))

#define MAP							\
LAMBDA(k,							\
	LAMBDA(f,					\
		CALL(FOLD, k, EMPTY, LAMBDA(l,		\
			LAMBDA(x,			\
				CALL(UNSHIFT, l, CALL(f, x)), \
			f, l),			\
		f)),					\
	k)							\
)

#define TEN	CALL(MUL, TWO, FIVE)
#define BBB	TEN
#define FFF	CALL(INC, BBB)
#define III	CALL(INC, FFF)
#define UUU	CALL(INC, III)
#define ZED	CALL(INC, UUU)

#define FIZZ	CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, EMPTY, ZED), ZED), III), FFF)
#define BUZZ	CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, EMPTY, ZED), ZED), UUU), BBB)
#define FIZZBUZZ CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, CALL(UNSHIFT, BUZZ, ZED), ZED), III), FFF)

char to_char(F c)
{
	return "0123456789BFiuz"[to_long(c)];
}

size_t length(F l)
{
	size_t len = 0;
	while (CALL(IF, CALL(IS_EMPTY, l), (F)0, (F)1)) {
		++len;
		l = CALL(REST, l);
	}
	return len;
}

char *to_string(F l)
{
	size_t len = length(l) + 1;
	char *p = malloc(len);
	if (!p)
		return NULL;
	len = 0;
	while (CALL(IF, CALL(IS_EMPTY, l), (F)0, (F)1)) {
		p[len++] = to_char(CALL(FIRST, l));
		//printf("char %d\n", p[len-1]);
		l = CALL(REST, l);
	}
	p[len] = 0;
	return p;
}

void print_strings(F l)
{
	while (CALL(IF, CALL(IS_EMPTY, l), (F)0, (F)1)) {
		printf("%s\n", to_string(CALL(FIRST, l)));
		l = CALL(REST, l);
	}
}

#define DIV				\
CALL(Z, LAMBDA(f,			\
	LAMBDA(m,		\
		LAMBDA(n,					\
			CALL(IF, CALL(IS_LESS_OR_EQUAL, n, m),	\
				LAMBDA(x,				\
					CALL(INC, CALL(f, CALL(SUB, m, n), n), x),	\
				f, m, n),			\
				ZERO),					\
		f, m),						\
	f)								\
))

#define PUSH				\
LAMBDA(l,				\
	LAMBDA(x,		\
		CALL(FOLD, l, CALL(UNSHIFT, EMPTY, x), UNSHIFT),	\
	l)				\
)

#define TO_DIGITS			\
CALL(Z, LAMBDA(f,			\
	LAMBDA(n,		\
		CALL(PUSH,	\
			CALL(IF, CALL(IS_LESS_OR_EQUAL, n, CALL(DEC, TEN)),	\
				EMPTY,						\
				LAMBDA(x,					\
					CALL(f, CALL(DIV, n, TEN), x),	\
				f, n)),					\
			CALL(MOD, n, TEN)),					\
	f)									\
))



int main(int argc, char *argv[])
{
#if 0
	printf("%s\n", CALL(LAMBDA(f, f), (F)"hello"));
	
	//printf("2 10: %ld\n", to_long(CALL(TWO, TEN)));
	printf("zero: %ld\n", to_long(ZERO));
	printf("one: %ld\n", to_long(ONE));
	printf("two: %ld\n", to_long(TWO));
	printf("three: %ld\n", to_long(THREE));
	printf("five: %ld\n", to_long(FIVE));
	printf("fifteen: %ld\n", to_long(FIFTEEN));
	printf("hundred: %ld\n", to_long(HUNDRED));

	printf("true: %s\n", to_boolean(TRUE));
	printf("false: %s\n", to_boolean(FALSE));
	
	printf("foo: %s\n", (char*)CALL(IF_0, TRUE, (F)"foo", (F)"bar"));
	printf("bar: %s\n", (char*)CALL(IF_0, FALSE, (F)"foo", (F)"bar"));
	printf("foo: %s\n", (char*)CALL(IF, TRUE, (F)"foo", (F)"bar"));
	printf("bar: %s\n", (char*)CALL(IF, FALSE, (F)"foo", (F)"bar"));

	printf("is_zero zero: %s\n", to_boolean(CALL(IS_ZERO, ZERO)));
	printf("is_zero three: %s\n", to_boolean(CALL(IS_ZERO, THREE)));
	
	printf("inc one: %ld\n", to_long(CALL(INC, ONE)));
	printf("add one three: %ld\n", to_long(CALL(ADD, ONE, THREE)));
	
	printf("dec three: %ld\n", to_long(CALL(DEC, THREE)));
	printf("sub 100 5: %ld\n", to_long(CALL(SUB, HUNDRED, FIVE)));
	
	printf("sub 5 3: %ld\n", to_long(CALL(SUB, FIVE, THREE)));
	printf("sub 3 5: %ld\n", to_long(CALL(SUB, THREE, FIVE)));
	printf("mul 3 2: %ld\n", to_long(CALL(MUL, THREE, TWO)));
	printf("pow 3 3: %ld\n", to_long(CALL(POW, THREE, THREE)));
	
	printf("1 <= 2: %s\n", to_boolean(CALL(IS_LESS_OR_EQUAL, ONE, TWO)));
	printf("2 <= 2: %s\n", to_boolean(CALL(IS_LESS_OR_EQUAL, TWO, TWO)));
	printf("3 <= 2: %s\n", to_boolean(CALL(IS_LESS_OR_EQUAL, THREE, TWO)));

	printf("3 mod 2: %ld\n", to_long(CALL(MOD, THREE, TWO)));
	printf("3 mod 1: %ld\n", to_long(CALL(MOD, THREE, ONE)));
	printf("3 mod 5: %ld\n", to_long(CALL(MOD, THREE, FIVE)));
	printf("3^3 mod (2+3): %ld\n", to_long(CALL(MOD, CALL(POW, THREE, THREE), CALL(ADD, THREE, TWO))));
	{
		F my_list = CALL(UNSHIFT,
				CALL(UNSHIFT,
					CALL(UNSHIFT, EMPTY, THREE),
					TWO),
				ONE);
		printf("first: %ld\n", to_long(CALL(FIRST, my_list)));
		printf("first rest: %ld\n", to_long(CALL(FIRST, CALL(REST, my_list))));
		printf("first rest rest: %ld\n", to_long(CALL(FIRST, CALL(REST, CALL(REST, my_list)))));
		printf("is_empty my_list: %s\n", to_boolean(CALL(IS_EMPTY, my_list)));
		printf("is_empty empty: %s\n", to_boolean(CALL(IS_EMPTY, EMPTY))); 
	}
	{
		F my_range = CALL(RANGE, ONE, FIVE);
		printf("first: %ld\n", to_long(CALL(FIRST, my_range)));
		printf("first rest: %ld\n", to_long(CALL(FIRST, CALL(REST, my_range))));
		printf("first rest rest: %ld\n", to_long(CALL(FIRST, CALL(REST, CALL(REST, my_range)))));
		print_list(my_range);
	}
	
	printf("fold(range one five)zero add: %ld\n", 
		to_long(CALL(FOLD, CALL(RANGE, ONE, FIVE), ZERO, ADD)));
	printf("fold(range one five)one mul: %ld\n", 
		to_long(CALL(FOLD, CALL(RANGE, ONE, FIVE), ONE, MUL)));
	printf("map(range one five)inc: ");
	print_list(CALL(MAP, CALL(RANGE, ONE, FIVE), INC));
	
	printf("%s\n", to_string(FIZZ));
	printf("%s\n", to_string(BUZZ));
	printf("%s\n", to_string(FIZZBUZZ));
	printf("%s\n", to_string(CALL(TO_DIGITS, FIVE)));
	printf("%s\n", to_string(CALL(TO_DIGITS, CALL(POW, FIVE, THREE))));
#endif
#if 1
	/* Fizz Buzz! */
	print_strings(CALL(MAP, CALL(RANGE, ONE, HUNDRED), LAMBDA(n,
		CALL(IF, CALL(IS_ZERO, CALL(MOD, n, FIFTEEN)),
			FIZZBUZZ,
		CALL(IF, CALL(IS_ZERO, CALL(MOD, n, THREE)),
			FIZZ,
		CALL(IF, CALL(IS_ZERO, CALL(MOD, n, FIVE)),
			BUZZ,
		CALL(TO_DIGITS, n)
		)))
	)));
#endif
	return 0;
}
