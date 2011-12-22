# Lambda Calculus with Church Encoding in C

Inspired by Tom Stuart's [Programming with Nothing](http://experthuman.com/programming-with-nothing),
I decided to port it to C.

### Problem: C doesn't have lambda functions.

Then let's create them.  Our lambda functions will accept only one argument, since [currying](http://en.wikipedia.org/wiki/Currying)
can be used to do multiple arguments.  We'll use a `typedef` `F` to denote a function taking one argument.

    typedef (void*)(*F)(void*);

GCC supports [nested functions](http://gcc.gnu.org/onlinedocs/gcc/Nested-Functions.html):

    F lambda() {
        void *func(void *arg) {
            return arg;
        }
        return func;
    }

Next, we need to be able to capture (bind) variables to the nested function.
Since the variables we might wish to capture may lie outside of the lambda function, 
we should rewrite it as a macro, and use a
[statement expression](http://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html)
containing the function.

    #define LAMBDA(_arg, _body)  ({               \
        void* _f_ ## _arg (void *_arg) _body      \
        _f_ ## _arg;                              \
        })

Now we can write nested lambda functions with variable capture.

    LAMBDA(x, {
        return LAMBDA(y, {
            return x;
        });
    })
        
The C preprocessor should expand this to:

    ({
        void *_f_x(void *x) {
            return ({
                void *_f_y(void *y) {
                    return x;
                }
                _f_y;
            });
        }
        _f_x;
    })
    
This has a problem, since `x` is on the stack when the outer `_f_x` is called, 
but will no longer exist on the stack when the inner `_f_y` is called.  In fact, the GCC documentation 
on nested functions strongly advises against accessing variables that are in the scope of a function that 
has returned.

### Problem: Captured variables can't be accessed on the stack

The captured variables will need to be captured manually and stored somewhere that gets passed with the function.  
Our lambda could accept an array of additional captured variables, and the function signature might look like this.

    void *_f_x(void *var[], void *x)
            
But then we need to somehow bind that var array to the function pointer.  Perhaps create a 
hash on function pointers to var array.  Perhaps create a `struct` containing the function 
pointer and a captured variable array, like this.

    typedef struct _func *F;
    struct _func {
        F (*call)(F[], F);
        F var[0];
    };

When we allocate the `struct _func` we'll have to make sure to allocate extra space for the
[zero-length array](http://http://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html)
to hold the captured variables.
Unfortunately, this is no longer callable as a plain C function, and we'll have to use some
indirection to get to the function pointer and pass the var array.

    #define CALL(f,x) (f)->call((f)->var, x)

There is a problem with this macro since it evaluates `f` twice, and this means that 
`f` gets expanded much more than it needs to be, resulting in bloated code and increased computation, but
will should produce the same result.  We'll fix it later.

Now our `LAMBDA` macro needs to accept a list of variables to capture.  Let's use a [variadic macro](http://http://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html).

    #define LAMBDA(_arg, _body, ...)  ({                \
        F _f_ ## _arg (F var[], F _arg) _body           \
        F _var[] = { __VA_ARGS__ };                     \
        F _this;                                        \
        _this = malloc(sizeof(*_this) + sizeof(_var));  \
        _this->call = _f_ ## _arg;                      \
        memcpy(_this->var, _var, sizeof(_var));         \
        _this;                                          \
        })

This is pretty bad.  We don't know how many captured variables there are, so they get copied to a temporary 
array.  The size of this array is then used to calculate the size of the struct.
Then the temporary array gets copied to the array inside the struct.

Now we can write the nested lambda like this, and it should work.

    LAMBDA(x, {
        return LAMBDA(y, {
            return var[0];
        }, x);
    })

More nesting would look like this.

    LAMBDA(f, {
        return LAMBDA(x, {
            return LAMBDA(y, {
                return CALL(var[0], CALL(var[1], y));
            }, var[0], x);
        }, f);
    })

This is ok as long as you can remember that `var[0]` refers to `f` and `var[1]` refers to `x` 
and to which scopes those rules apply.

### Problem: captured variables array is difficult to use in nested lambda functions

So let's declare a new struct for each lambda containing the names of the captured parameters, and pass this 
struct to the nested function.  The struct can be anonymous, and we'll use 
[typeof](http://gcc.gnu.org/onlinedocs/gcc/Typeof.html) and 
[compound literals](http://http://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html)
to initialize it.

    #define LAMBDA(_arg, _body, ...)  ({                  \
        struct {                                          \
            struct _func _f;                              \
            F _var[0], ##__VA_ARGS__;                     \
        } *_this;                                         \
        F _f_ ## _arg (F _arg, typeof(_this) this) _body  \
        _this = malloc(sizeof(*_this));                   \
        *_this = (typeof(*_this)){{_f_ ## _arg},{},##__VA_ARGS__};  \
        (F)_this;                                         \
        })

Now we can write the more nested lambda function like this.

    LAMBDA(f, {
        return LAMBDA(x, {
            return LAMBDA(y, {
                return CALL(this->f, CALL(thix->x, y));
            }, this->f, x);
        }, f);
    })

That doesn't compile.  It's because `this->f` is used as a member name within the struct, so we need to rewrite it.

    LAMBDA(f, {
        return LAMBDA(x, {
            F f = this->f;
            return LAMBDA(y, {
                return CALL(this->f, CALL(thix->x, y));
            }, f, x);
        }, f);
    })

It's better, in that we can use `this->`-prefixed names for captured variables, but worse since we need to copy them to 
unprefixed variables in order to be captured again.  When many variables are captured to the innermost function, this
becomes extremely verbose.  Also, forgetting to capture a variable and forgetting the prefix will compile cleanly,
and will attempt to access the scope of the returned outer function, which is very bad.

### Problem: captured variables still too unwieldy

We don't want to unpack a var array, and we don't want to use a prefix, so how 
about we pass all the captured variables as arguments to the lambda function?

How?  Indirectly.  Call another dispatching function which unpacks a var array for us, 
which then calls the lambda function with those vars.  We'll need a dispatch function
for each number of captured varibles we're going to allow.  Let's say we allow up to eight captured
variables.  We add a union containing a function pointer for each number of variables.

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

The `call` will hold the dispatch function, and the `call_N` will hold the lambda function.
Now define some dispatch functions for each and put them into an array.  We can add more
dispatch functions if we need more captured variables later.

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

Now for the macro:

    #define LAMBDA(_arg, _expr, ...) ({   \
        F _f (_arg, ##__VA_ARGS__) F _arg, ##__VA_ARGS__; { return _expr; } \
        struct {struct functor _func; F _a[0], ##__VA_ARGS__; } *_this;      \
        _this = malloc(sizeof(*_this));    \
        *_this = (typeof(*_this)){    \
                {lambda_dispatch[(sizeof(*_this)-sizeof(_this->_func))/sizeof(F)], _f},     \
                {}, ##__VA_ARGS__};     \
        (F)_this;      \
        })

Note our lambda function uses K&R C syntax for declaring the function arguments, since we have no way
of interleaving the type `F` into `__VA_ARGS__`.
Note also that the arguments are declared in the struct alongside a zero-length array, 
so that if there are no capture variables, the token pasting operator will drop the comma.
If we were to attempt to initialize the zero-length array directly, GCC will rightly complain
about excess initializers.
The lambda dispatch function is selected by subtracting the size of the inner struct from the anonymous struct,
which gives the size of the captured variables.
Also the lambda function body is now passed as an expression rather than a statement block, which frees us
from putting the braces and return statement in our lambda definitions.

It would be nice if the `CALL` macro could accept multiple arguments, instead of having to nest multiple `CALL` expressions.
Also there's the problem mentioned earlier about evaluating the `f` expression twice.
Here's a helper function `call` which takes variable arguments and a terminator `_` so we know when to stop.
Unfortunately `call` is iterative, and there's nothing we can do about it in C.
Hopefully we can reason that it is equivalent to purely functional calling.
    
    #define CALL(f, ...) call(f, __VA_ARGS__, _)
    F _ = (F)-1; /* call arg terminator */
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

Now we can do

    LAMBDA(f, 
        LAMBDA(x,
            LAMBDA(y,
                CALL(f, x, y),
            f, x),
        f)
    )

This isn't bad.  Almost elegant, and not what you'd normally expect to see in a C program.

Now that we have lambda functions...

## Lambda Calculus

### Church encoding

Here is a function representing zero using [Church encoding](http://en.wikipedia.org/wiki/Church_encoding)

    #define ZERO    	\
    LAMBDA(p,		    \
    	LAMBDA(x, x)	\
    )

For other numbers, we use this `P` macro, to concisely call the function `p`.

    #define P(x) (p->call(p, x))
    
    #define ONE    	\
    LAMBDA(p, 		\
    	LAMBDA(x,	\
    		P(x),	\
    	p)		    \
    )
    
    #define TWO			\
    LAMBDA(p, 			\
    	LAMBDA(x,		\
    		P(P(x)),	\
    	p)			    \
    )
    
    #define THREE		\
    LAMBDA(p,			\
    	LAMBDA(x,		\
    		P(P(P(x))),	\
    	p)			    \
    )

Now a function to convert a Church-encoded function-number back to C.
A `long` is assumed to be the same size as a pointer, `uintptr_t` should really be used instead.

    long to_long(F f)
    {
        F inner(F p, F x)
    	{
    		return (F)((long)x + 1);
    	}
    	struct functor lambda = {inner};
    	return (long) CALL(f, &lambda, 0);
    }

Now we can try it out.

    printf("zero: %ld\n", to_long(ZERO));
	printf("one: %ld\n", to_long(ONE));
	printf("two: %ld\n", to_long(TWO));
	printf("three: %ld\n", to_long(THREE));

    zero: 0
    one: 1
    two: 2
    three: 3

It actually works.

