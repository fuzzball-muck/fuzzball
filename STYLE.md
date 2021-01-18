# Style Documentation

The purpose of this documentation is to describe the coding style used by the Fuzzball Server project.  This style must be used by any code contributions made to the project in order to make things consistent and to ensure the code remains property documented.

This document is a little daunting and there's a lot of stuff in here.  Don't worry too much -- try to follow the established guidelines of the existing code and you will be fine.  This document just serves as an exhaustive list of ideals.  The biggest thing to pay attention to is how we document code; we use Doxygen auto-generated documentation and a significant effort was put into building the documentation we have today.  We want to make sure that level of documentation quality continues going forward, so if you only read one section of this, read the Documentation section!

## Lines

Lines are sized to aproximately fit an 80 character line width.  It is preferred, if possible, to break long lines up into multiple lines with appropriate spacing such that things line up nicely.

This rule is not strictly adhered to; if is significantly more readable to use a long line, or the line goes only slightly over the 80 character limit, it is preferred to do whatever is more readable.

We use UNIX style line endings in all files.

The use of "phrasing" or using blank lines to separate groups of like code is highly encouraged.  For instance, this would be frowned upon:

```
    int x = 0;
    x += 1;
    x += y;
    if (x) {
        x += 3;
    }
    while (x) {
        x--;
    }
```

Instead, it would be preferred to have code spaced out into logical groupings such as:

```
    int x = 0;

    x += 1;
    x += y;

    if (x) {
        x += 3;
    }

    while (x) {
        x--;
    }
```

Initialization, related operations, and control structures would each be their own 'phrase' or 'paragraph' of code.

## Spacing

With regards to spacing, we follow the following rules:

* Indentation is done with 4 spaces.  Tab characters are not used.
* Each level of nested control structure (for, if, while, etc.) increases the indent level.  As a rule of thumb, any time a ```{``` is encountered, he indent level increases by one, and any time a ```}``` is encountered, the indent level decreases by one.
* Coders are encouraged to use additional spacing as needed to make things line up in a fashion that is easy to follow.  For instance, an 'if' statement that is broken up across multiple lines may be lined up thusly:

```
    if (x == 1 && y == 1 && z == 1 && .......
        a == 1 && b == 1 &&) {
```

* Control structures, such as if, while, for, etc. with the notable exception of functions will have a space between the control keyword and the parenthesis.  i.e. we would do ```if (...) {``` instead of ```if(...) {```.  Function notes will be described in their own section.
* Spaces are used after the hash symbol to nest precompiler definitions.  In this case, only a single space per level of indentation is done.  For instance:

```
#ifdef WIN32
# define O_NONBLOCK 1
#else
# if !defined(O_NONBLOCK) || defined(ULTRIX)
#  ifdef FNDELAY /* SUN OS */
#   define O_NONBLOCK FNDELAY
#  else
#   ifdef O_NDELAY /* SyseVil */
#    define O_NONBLOCK O_NDELAY
#   endif
#  endif
# endif
#endif
```

* There are spaces surrounding most operators.  For instance, ```x = 1``` instad of ```x=1```
* The exception is the pointer operator, ```*```, which is usually afixed to the variable name.  For instance: ```char *test;``` instead of ```char * test;```  The dereference operator, ```&```, follows this rule as well as the not operators, ```!``` and ```^```.
* Additionally, extra spaces are not used with array brackets or parenthesis.  For instance, ```x[123];``` instead of ```x [ 123 ];```, or ```(x + y + z)``` not ```( x + y + z )```.

## Code Blocks / Curly Brackets

The use of curly brackets ```{ }``` follows the following rules:

* For function definitions, the function will both start and end with a curly bracket on a line by itself.  For instance:

```
static int
function_name(...)
{
    first line;
    ...
}
```

* If curly braces are used to form a block of code independent of a control structure, then the opening and closing braces start on lines by itself.  For instance:

```
static int
function_name(...)
{
    some code;
    other code;
    ...
    {
       some block of code not related to a control structure;
    }
    ...
    more code
}
```

* When used with a control structure (while, if, for, etc.), the opening brace is always inline with the structure, preceeded by a space, and the closing brace is always on a line by itself.  For instance:

```
    if (...) {
       code;
    }
```

* In general, it is preferred that all control structures -- even those that contain only a single line -- use curly braces as demonstrated above.  However, this isn't strictly required.  Generally, however, we will reject code of the format:

```
    if (...) code;
```

Inline 'if' statements using the ? and : operators are permitted as long as the complexity level is not too extreme.

* The only exception here is precompiler defines that require braces.  Frankly, this is a little bit of a free-for-all because multi-line compiler definitions are pretty ugly to deal with.  Try your best to make it look nice, and consider using a function instead if your compiler definition is so complex that it needs curly braces.

## Functions

* Functions should be named in all lower case, using underscores to separate words.  Camel case will be rejected, unless it is already extensively used in the module you are working on -- there are some areas of the code that are camel case and are "grandfathered" in, but that is not our preference for new code.
* Function names should be descriptive and reasonably certain to be unique in the codebase.  There are no specific name formatting requirements for MOST functions.
* The exception is MUF and MPI primitive handlers.  These functions should be named after the implemented primitive.  For a MUF primitive named 'EXAMPLE', you would name the function: prim_example.  For an MPI macro named 'EXAMPLE', you would name the function: mfn_example.
* There are other exceptions as well; some parts of the code have their own naming schemes, however the majority do not.  Please try to adhere to naming conventions when they are apparent.
* For MUF and MPI primitives that use special characters that are valid in MUF/MPI but cannot be used in C functions, such as ! or ?, substitutes are used.  "bang" is the substitute for !, "at" the substitute for @, and "p" is the substitute for ?.  For other symbols, it is typical to type out the name of the symbol; you are encouraged to conform with what the developers that have come before you have done.
* Function declarations should always include variable names and should always match the variable names of the definitions.  This is a requirement for Doxygen.  So, a delcaration should look like this:

```
int func_name(int player, int room);
```

And NOT like:

```
int func_name(int, int);
```

* Function declarations have the return type inline with the function name and arguments as demonstrated above.
* Function definitions have the return type on a line before the function name.  The parameter variable names must match the declaration.  So the definition for the above function would look like this:

```
int
func_name(int player, int room)
{
    ....
}
```

* As noted in the section on braces, the start and end braces for a function definition are on their own lines.
* ALL FUNCTIONS with NO EXCEPTIONS will be preceeded by a documentation block.  Please see the section on documentation for more details.
* Single line functions that exist solely to wrap other functions will be carefully scruitinzed and are generally frowned upon unless there is a strong case for them.  These wrapper functions often make the code harder to follow.  For instance:

```
int
some_long_function(int x, int y, int z)
{
    /* Lots of code */
}

int
some_small_function(int x, int y)
{
    return some_long_function(x, y, 0);
}
```

Generally speaking, we will reject code with this structure unless there is a clear reason why 'some_small_function' adds notable value.

* The 'static' keyword should be used for any function that is considered 'private' to a given file.  Due to the size of the code base, liberal use of the 'static' keyword for file-specific global variables and functions is highly encouraged to ensure a lack of conflict.
* Functions must be designed with threading in mind.  At present, the MUCK is not multithreadd, but that could change in the future.  Code that blatantly violates reenterancy will not be accepted.  If mutexes would be needed for a critical section, that section should be commented as potentially needing mutexes in the future.
* Functions that use recursion must have a reasonable termination condition.  Recursive functions that have any chance of hanging the MUCK server will be rejected.

## File Headers

All .h and .c files in the project will have a file header.  This is required by Doxygen -- if the header is missing, Doxygen will ignore the file.  It also gives us the opportunity to put in the copyright notice.

Unless there's some unique usage information for the file, the doc block can be brief.  For example, here is a sample header for a .h file:

```
/** @file array.h
 *
 * Header for the different array types used in Fuzzball.  This is primarily
 * used for MUF primitives. 
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
```

The formatting here is important, and the file name after the ```@file``` must be the correct file name.  Note the copyright notice as well.  Here's a sample .c header:

```
/** @file array.c
 *
 * Source for the different array types used in Fuzzball.  This is primarily
 * used for MUF primitives. 
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
```

Just copy/paste these samples, alter the file name, and alter the descriptive sentence and you're good to go.

## Comments and Documentation

This project is a "generational" project that has been worked on by many developers across decades.  As such, making code that anyone can come and work on in the future is very important.  As such, a large effort was put into doing extensive documentation of the code.

We use Doxygen to generate easily browsable API documentation.  As such, every function must have a documentation header.  Documentation headers look like this:

```
/**
 * Some notes about the function
 *
 * You can reference another function thusly:
 * @see some_function_name
 *
 * @TODO Some note about stuff you need to do later
 * 
 *
 * @param param1 notes about the parameter
 * @param param2 notes about the parameter
 * @return notes about what is returned (leave this off for void functions)
 */
int
some_function(param1, param2);
```

If the function is private (static) then add an @private tag thusly:

```
/**
 * Function notes
 *
 * @private
 * @param ...
 */
```

These doc blocks are put on both the definition and the declaration.  Yes, that is redundant, but that is what we decided to do.  Better too much documentation than too little.

IMPORTANT: if a function deals with memory, the documentation must make it clear who's responsibility it is to free the memory.  For example:

```
/**
 * This allocates a string with 'x' bytes size.
 *
 * You are responsible for freeing this memory.
 *
 * @param x size of string to allocate
 * @return allocated string
 */
char* allocate_string(int x);
```

You can also use ```@see``` to direct the user to see a deallocation function.  The server software makes liberal use of static buffers so it isn't always obvious if a function returns a pointer to a static buffer (which should not be free'd by the caller) or something that allocates memory that must be freed by the caller.

Variables are also documentated with Doxygen.  For exportable global variables, they are documented thusly:

```
/**
 * @var description of variable
 */
int variable;
```

For static variables, use the private tag:

```
/**
 * @private
 * @var description of variable
 */
int variable;
```

As a final note for documentation, it is highly encouraged to document any complex portion of code within functions.

