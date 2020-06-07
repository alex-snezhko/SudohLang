# Sudoh Language Documentation

## Preface
This document contains documentation of the Sudoh programming language, with included code samples to better
illustrate Sudoh's syntax/features. Sudoh is a simple language, and any prior experience with essentially any high-level
programming language should make getting started with Sudoh an easy task.


## Basic information
Sudoh is a strongly-typed, dynamically type-checked language. There are 6 types in Sudoh, and they are:
- Number
- String
- Boolean
- List
- Object
- Null

### Statements

Each statement in Sudoh must be on a new line, and a valid line is either: a variable assignment, a procedure call, or a
programming structure declaration statement. Here is a code segment illustrating basic variable assignment.
```
// This is a comment, used for code documentation. A comment is initiated using '//', and everything
// after the '//' on a line is ignored by the sudohc compiler and treated as part of the comment.

// A variable may be named with an identifier consisting of any combination of uppercase or lowercase letters,
// numbers, and underscores (with the exception that the first character of the name must not be a number).

a <- 123         // this is an assignment of the value '123' into a new variable named 'a'
a <- "hello"     // we are now reassigning 'a' to contain the string value "hello"
```

It is possible for a single statement/expression to be split into multiple lines, for the primary reason of avoiding
extremely long lines in complex conditions, arithmetic expressions, etc. However, there are some rules associated with this.
Firstly, a statement may only be split into multiple lines immediately after a binary operator e.g. `+` or `and`, after a comma,
or after the beginning brace/before the ending brace of a list/object literal. On top of that, all lines of a multiline statement
following the first must be indented at least one level higher than the first line.

These are some statements containing valid multiline expressions:
```
longArithmetic <- (1 + 2 - 3) * (4345 / 18 + 871) -
    456 + ((123 - 44) + 12345)

if (condition1 and condition2 and condition3) or
    (condition4 and condition5 and condition6) then
    print("conditions met")

largeList <- [
    "string 1",
    "string 2",
    "string 3",
    "string 4",
    "string 5"
    ]

largeObject <- {
    "value1" <- 42,
    "value2" <- null,
    "8000" <- 8001,
    "hello" <- "world"
    }
```

These are some statements containing invalid multiline expressions:
```
longArithmetic <- (1 + 2 - 3) * (4345 / 18 + 871) -
456 + ((123 - 44) + 12345)

if (condition1 and condition2 and condition3) or
(condition4 and condition5 and condition6) then
    print("conditions met")

largeList <- [
    "string 1",
"string 2",
    "string 3",
"string 4",
    "string 5"
    ]

largeObject <-      // cannot end line after '<-'
{
    "value1" <- 42,
    "value2" <- null,
    "8000" <- 8001,
    "hello" <- "world"
}
```

### Type checking

Some operations in Sudoh are only valid if performed on values of particular types. As the type of a variable
may change throughout the execution of a program, it is important that the programmer keeps track of variable types
and ensures that the operations performed on data are valid.
```
var <- 123
result <- var * 2    // valid operation; result = 246

var <- "hello"       // reassign 'var' to contain a string value
result <- var * 2    // same operation as before, but it is no longer valid as multiplication may only be performed on numbers
```

The type of a value/variable can be checked at runtime with the `type` procedure, which may be useful in verifying
the integrity of operations before performing them, or for other uses
```
rightOperand <- // some value

// perform arithmetic if 'rightOperand' is number, error message otherwise
if type(rightOperand) != "number" then
    printLine("right should be number")
else
    printLine(2 * rightOperand)

list <- ["a", 2, "b", null, "c"]
stringList <- []
for each e in list do
    if type(e) = "string" then
        append(stringList, e)

// stringList = ["a", "b", "c"]
```

### Values/References
Sudoh's number, string, and boolean types are value types which use value semantics, while the others are reference types which
use reference semantics. If a variable (`a`, for example) is set to a reference type and that reference type is
modified at another point in the program e.g. a list has an element added to it, that change will be seen by `a`
```
str1 <- "a"    // strings are value types
str2 <- str1
str2 <- str2 + "b"
// at this point 'str1' = "a" and 'str2' = "ab"

a <- [1, 2]    // lists are reference types
b <- a
b[2] <- 3
// at this point both 'a' and 'b' refer to a list '[1, 2, 3]'
```

### Scope
It is important to understand the concept of variable 'scope' to use Sudoh correctly. The scope level of Sudoh statements
is indicated by their indentation level on a line (note: must use tabs for indentation). Sequential statements in the same
block must maintain the same indentation level.
```
a <- 1
b <- a + 3
    c <- a + b    // illegal; statements in the same block must be indented on the same level
```

The only time it is legal to indent a statement higher than the previous one is when entering a new block: when entering an
`if` statement block, for example (more info on structures including `if` later).
```
if true then
    // all statements indented with one tab are in the scope of the 'if' statement
    a <- 3

// a statement with indentation of 0 tabs; this statement is not in the 'if' block,
// and indicates that the 'if' block is complete
b <- "hello"
    c <- "asdf"   // invalid; the current valid scope level is 0 tabs (from the previous statement)

if true then
        a <- 3    // 2-tab indentation, with 'if' statement at 0 tabs. Illegal: statements in block must be indented exactly one tab higher
```

Variables declared inside of a scope that is no longer valid are also no longer valid.
```
if true then
    a <- "hello"   // this statement will create a new variable 'a' in the scope of the 'if' statement block

hello <- a         // illegal; 'a' no longer exists as its scope was the 'if' statement block, which has now been completed

// Alternative

a <- ""            // this statement will create a new variable 'a' holding the initial value of an empty string
if true then
    a <- "hello"   // this statement will reassign the value of 'a' to "hello"

hello <- a         // this is now legal; a was previously declared at the current scope
```

## Types

### Number
One of the more simple types in Sudoh is the Number. A number may contain any positive or negative real number value decimal (bound
by the user's double-precision floating point number boundaries). Sometimes in a Sudoh program, only integer numbers are acceptable
for certain operations e.g. indexing into a list. To handle this, a number which is *close enough* to an integer (+/- 0.00001) is
treated as one. There are 5 valid arithmetic operators that can be used on two numbers: `+` (addition), `-` (subtraction), `*`
(multiplication), `/` (floating point division), and `mod` (modulus division). The precedence of arithmetic expression evaluation is
parentheses -> multiplication/division -> addition/subtraction.
```
a <- 1 + (2 * 3) - 4   // a = 3
m <- 5 mod 2           // m = 1
p <- -1.5 * -2         // p = 3
```

### String
Strings are lists of characters. String literals are defined by a set of characters between quotation marks ("). Strings may be
concatenated together using the `+` operator. Individual characters of a string may be accessed, however, individual characters
of an existing string may not be modified as Sudoh strings are *immutable*. The number of characters in a string may be found by
using the `length` function.
```
hello <- "Hello "
world <- "World!"
helloWorld <- hello + world    // helloWorld = "Hello World!"
length(helloWorld)             // 12

// syntax to access a character of a string: <string>[<integer index>]
w <- helloWorld[6]             // w = "W"; note: the type of 'w' is also 'string' (Sudoh has no notion of a 'character' type)

helloWorld[1] <- "a"           // illegal; Sudoh strings are immutable
withNum <- "asdf" + 1 + true   // legal; concatenations between strings and other types are valid
                               // withNum = "asdf1true"
```

### Boolean
The boolean type is used primarily for program logic flow purposes, such as in `if` or `while`/`until` conditions. A boolean may
possess values of either `true` or `false`. Compound conditions may be formed with the `and`/`or` operators and may be inverted
with `not`.
```
true and false       // false
true or false        // true
not false            // true
not false and false  // false
```

#### Comparisons
Variables/values in Sudoh may be compared to other variables/values. The result of such a comparison is a boolean value indicating
whether the comparison was accurate. There are 6 comparison operators in Sudoh. They are:
-   `=` (is equal to); reference comparison on reference types, value comparsion between value types;
    valid between variables of the same type, or between a variable and `null`
-   `!=` (is not equal to); similar usage to `=`; outputs opposite result of what `=` would
-   `<` (is less than); if performed between two numbers, returns whether left number is less than right;
    if performed between two strings, scans strings character-by-character and outputs `true` if ASCII value(`<left>[n]`) <
    ASCII value(`<right>[n]`)
-   `<=` (is less than or equal to); similar in usage to `<`, but also outputs `true` if the values being compared are equal
-   `>` (is greater than); outputs opposite result of `<=`
-   `>=` (is greater than or equal to); outputs opposite result of `<`

### List
Lists in Sudoh are variable-length collections of elements. Variables of varying types may be placed into the same list.
Elements may be inserted into a list using the `append` or `insert` functions and removed with the `remove` function. The contents
of a list may be either accessed, or modified with bracket notation as well. The number of elements in a list may be found
by using the `length` function
```
empty <- []         // declare an empty list
length(empty)       // 0
list <- [1, 2, 3]   // declare a list with 3 elements: 1, 2, and 3
length(list)        // 3
one <- list[0]      // one = 1
three <- list[2]    // three = 3
four <- list[3]     // invalid; attempt to access fourth element of a 3-element list

list[3] <- 4        // valid; attempt to set a list element outside of the current bounds is valid
                    // list = [1, 2, 3, 4]

list[5] <- 6        // if a list element is set at an index greater than the current size of the list,
                    // the intermediate list elements will be set to null
                    // list = [1, 2, 3, 4, null, 6]

list[4] <- 5        // list = [1, 2, 3, 4, 5, 6]
append(list, 7)     // list = [1, 2, 3, 4, 5, 6, 7]

insert(list, 1, 9)  // insert the element '9' at index '1' of 'list'
                    // list = [1, 9, 2, 3, 4, 5, 6, 7]

removeLast(list)    // list = [1, 9, 2, 3, 4, 5, 6]

remove(list, 0)     // remove element at index '0' from 'list'
                    // list = [9, 2, 3, 4, 5, 6, 7]

append(list, "hi")  // list = [9, 2, 3, 4, 5, 6, 7, "hi"]
```

### Object
Objects in Sudoh are associative data structures which map string 'fields' to values. Looking up a field in an object will return the
field's associated value. Values may be accessed or inserted into an object using bracket notation, and removed with the `remove`
function. The number of elements in an object may be found using the `length` function
```
empty <- {}            // declare an empty object
length(empty)          // 0

// declare an object with 2 fields: "hello" = 'null', "42" = "world"
object <- { "hello" <- null, "42" <- "world" }

length(object)            // 2
object["third"] <- 2      // "hello": null, "42": "world", "third": 2
object["hello"] <- "hi"   // "hello": "hi", "42": "world", "third": 2
remove(object, "hello")   // "42": "world", "third": 2
hello <- object["hello"]  // invalid; field "hello" in 'object' does not exist
```

### Null
Null in Sudoh is a special type which represents an object which does not have a value. The only possible value of type 'null'
is `null`. Most operations on `null` values are intentionally undefined, and will cause a runtime exception if attempted.
Some possible uses for `null` can be to indicate that a variable has not yet been assigned a value, or as a special output value
in a procedure to indicate that something went wrong (more on procedures later).
```
num <- null                // indicate that 'num' is initially undefined
inp <- number(input())     // more on the 'number' and 'input' procedures in the 'Sudoh standard library' section below
if inp < 3 then
    num <- 3
else if inp > 4 then
    num <- 4

// most operations performed on 'num' will be invalid if the input number was in the range [3, 4]
```


## Programming structures

### `if` statements
The `if` statement is a structure that checks a condition and will run the following block if the condition evaluates to true
```
condition <- true
if condition then
    // statements
```
An `if` statement block can optionally be followed by any number of `else if` statements and/or one `else` statement.
Only one block in an `if`-`else` structure will be run, depending on which sequential condition is found to be true first.
```
cond1 <- false
cond2 <- true
if cond1 then
    print("first")
else if cond2 then
    print("second")
else
    print("third")

// 'second' will be printed
```

### `while`/`until` loops
The `while`/`until` loops are structures that run a block of code *while* a condition is true or *until* a condition is true
```
// this loop will run 5 times
i <- 0
while i < 5 do
    i <- i + 1

// this loop will run 4 times
str <- ""
until str = "aaaa" do
    str <- str + "a"

// infinite loop
while true do
    // statements
```

### `for` loops
The `for` loop is a structure which runs a block of code a certain number of times, and keeps a variable with information
about the current iteration of the loop. The structure of a `for` loop statement is as follows:
`for [iteration variable] <- [initial value] (down)? to [final value] do`
```
num <- 5
for i <- 0 to num do
    print(i)
// '012345' will be printed

for i <- num down to 0 do
    print(i)
// '543210' will be printed

for i <- 0 to -1 do
    print(i)
// nothing will be printed
```
Sudoh also has a `for each` loop which iterates over all of the elements of a collection (string, list, or object) and runs
a block of code for each element in the collection. The structure of a `for each` loop statement is as follows:
`for each [iteration variable] in [collection] do`. `for each` loop on a string: iterate over each character in the string;
on a list: iterate over each element in the list; on a object: iterate over each field name of the object (in lexical order)
```
for each c in "asdf" do
    print(c + " ")
// 'a s d f ' will be printed

list <- [0, 1, 2, 3]
for each e in list do
    print("[" + e + "]")
// '[0][1][2][3]' will be printed

object <- { "a" <- 0, "c" <- "apple", "b" <- null }
for each key in object do
    print(string(key) + ": " + object[key] + "  ")
// 'a: 0  b: null  c: apple  ' will be printed
```

### `repeat` loops
The `repeat` loop is a structure that is similar to a `while`/`until` loop, with the difference that the code inside
the `repeat` block is guaranteed to run at least once; as opposed to a normal `while`/`until` loop which checks for
the condition before each iteration, the `repeat` loop will check the condition after each iteration, and will run
the code block again if it is evaluated to true. The structure of a repeat loop is shown in the examples:
```
i <- 0
repeat
    i <- i + 1
while i < 0
// 'i' will contain the value of 1 at the end of this loop

i <- 0
repeat
    i <- i + 1
until i != 0
// 'i' will contain the value of 1 at the end of this loop
```

### Extra loop statements: `continue` and `break`
There are 2 extra statements that are valid in the context of inside a loop: `continue` and `break`. The `continue`
statement is used to end a loop iteration preemptively, and the `break` statement is used to exit a loop altogether.
```
i <- 0
while true do
    if i = 5 then
        break      // exit the 'while' loop entirely
    i <- i + 1

// i = 5 at this point

for i <- 1 to 3 do
    if i = 2 then
        continue   // end the current loop iteration preemptively
    print(i)

// '13' will be printed
```

It is important to note that `continue` and `break` only work for the innermostly-nested loop they are specified in
```
while true do
    while true do
        break      // this will only break out of the nested while loop
    printLine("iteration")

// the above loop will print 'iteration' indefinitely
```


## Procedures

Units of code may be broken up into 'procedures' for organization, understandability, and/or reusability. A procedure
is a segment of code that may be executed ('called') elsewhere in a program to run the code contained within the procedure; a
procedure that is never called will not do anything
A procedure optionally accepts one or more variables as input.
```
// define a procedure named 'printVars' which accepts 2 input parameters
// (handled with names 'a' and 'b' in the context of the procedure), and prints them both
procedure printVars <- a, b
    print(a)
    print(b)

printVars("hello", "world")   // call the 'printVars' procedure with the values "hello" and "world" as input
// 'helloworld' will be printed

// define a procedure named 'noInput' which does not accept any input parameters
procedure noInput
    printLine("this procedure accepts no parameters as input")

// call the input-less procedure
noInput()
```


### Procedure output
A procedure may explicitly output a value; for example, a procedure may be used to perform a computation and then output
the result of that computation.
```
// define a procedure which outputs the input number squared
procedure square <- num
    output num * num        // use the 'output' keyword followed by an expression to explicity output a value from the procedure

sq <- square(3)       // sq = 9

// a procedure which does not explicitly 'output' a value will output 'null'
procedure noOutput
    print("asdf")

val <- noOutput()   // val = null
```

A non-outputting procedure may also be terminated before reaching the end of the procedure with the `exit` keyword.
If a procedure reaches the `exit` keyword, it will implicitly output the value `null` as in a typical non-outputting
procedure.

```
procedure printSomething <- num
    if num < 5 then
        printLine("< 5")
        exit
    printLine(">= 5")

printSomething(4)      // only '< 5' will be printed
printSomething(6)      // only '>= 5' will be printed
```


### Extra procedure info
-   Procedures may **not** access 'global' variables, as is possible in some other programming languages.
    ```
    globalVar <- "hello"

    procedure printGlobal
        print(globalVar)   // illegal; code in a procedure may not access variables declared outside of the procedure
    ```

-   Procedures definitions do not have to be located before use of said procedures in a source code file. As long as
    they are defined at some point, they may be used as normal.
    ```
    // valid; procedure 'later' is defined later in the source code
    later(10)

    procedure later <- val
        print(val)
    ```

-   Nested procedures (procedures defined within procedures) are illegal in Sudoh.
    ```
    procedure proc
        procedure nested       // illegal
            print("nested")

        print("proc")
    ```

-   Several procedures with the same name may be defined, so long as they do not accept the same number input parameters
    ```
    procedure proc <- p1
        printLine(p1)

    procedure proc <- p1, p2
        print(p1)
        printLine(p2)

    // call 'proc' accepting one parameter
    proc("asdf")
    // call 'proc' accepting two parameters
    proc("as", "df")
    ```

-   Parameters are passed into procedures by *object reference*.
    ```
    procedure modifyParams <- num, list1, list2
        num <- 123
        list1 <- ["new", "list"]
        append(list2, 3)

    num <- 5
    list1 <- ["some", "stuff"]
    list2 <- [0, 1, 2]
    modifyParams(num, list1, list2)
    
    // num = 5
    // list1 = ["some", "stuff"]
    // list2 = [0, 1, 2, 3]
    ```


## Sudoh standard library
Sudoh contains several built-in standard library procedures which either provide functionality that cannot be
performed natively with Sudoh, or convenience for performing common operations.

### `print` and `printLine` input: `var`
`print` is a procedure which will print the value of a variable to console output. The variable passed in is
implicitly converted to a string (see 'string' procedure) and then printed. `printLine` is identical in behavior
to `print`, with the exception that a newline character will be printed afterwards as well.
```
num <- 3.14
print(num)
print(num)      // '3.143.14' will be printed

printLine(num)
printLine(num)  // '3.14' will be printed twice on separate lines
```

### `input` output: string
`input` is a procedure which will return user keyboard input in the format of a 'string'
```
printLine("Enter some input.")
in <- input()         // user types in 'hello'; the value of 'in' is now "hello"
printLine("input: " + in)        // 'input: hello' will be printed
```

### `length` input: `collection`; output: integer
`length` is a procedure which outputs the number of elements in a collection. For a string, `length` will
return the number of characters in the string. For a list, `length` will return the number of elements in the
list. For an object, `length` will return the number of fields in the object.
```
str <- "asdf"
length(str)        // 4

list <- [1, 2, 3]
length(list)       // 3

object <- { "a" <- 1, "b" <- 2}
length(object)        // 2

length(2)          // invalid; parameter must be a collection
```

### `string` input: `var`; output: string
`string` is a procedure which outputs a string representation of the input variable.
```
string(123)                     // "123"
string(true)                    // "true"
string("asdf")                  // "asdf"
string([1, 2, 3])               // "[ 1, 2, 3 ]"
string({"a" <- 1, "b" <- 2})    // "{ a <- 1, b <- 2 }"
string(null)                    // "null"
```

### `number` input: `str`; output: number
`number` is a procedure which parses a string containing a numerical value into a number. If the specified
string does not contain a number value, the procedure will output `null`
```
a <- number("123")
b <- number("4.5")
c <- a + b                 // c = 127.5

invalid <- number("asdf")  // invalid = null
```

### `range` input: `indexable`, `begin`, `end`; output: (string|list)
`range` is a procedure which outputs a portion of the specified string or list, beginning at index `begin`
and ending before index `end`
```
str <- "abcdef"
first <- range(str, 0, 3)      // first = "abc"
second <- range(str, 3, 6)     // second = "def"

list <- [5, 4, 3, 2, 1]
part <- range(list, 1, 4)      // part = [4, 3, 2]
range(list, 0, 6)              // invalid; 'end' index greater than length of 'list'
```

### `integer` input: `num`; output: integer
`integer` is a procedure which outputs a truncated integer value of an input number.
```
integer(3.14)      // 3
integer(3)         // 3
integer("3.14")    // invalid; input must be a number
```

### `ascii` input: `code`; output: string
`ascii` is a procedure which outputs the ASCII character representation of the input ASCII code as a string.
```
ascii(65)     // "A"
ascii(122)    // "z"
ascii(33)     // "!"
```

### `random` input: `range`; output: integer
`random` is a procedure which outputs a random integer in the range of [0, `range`).
```
random(3)    // will return either 0, 1, 2
random(100)  // will return random number on interval [0, 100)
```

### `append` input: `list`, `element`
`append` is a procedure which adds a new element to the end of a list.
```
list <- [1, 2]
append(list, 3)    // list = [1, 2, 3]
append(list, 4)    // list = [1, 2, 3, 4]
```

### `insert` input: `list`, `index`, `element`
`insert` is a procedure which inserts an element into a list at a given index.
```
list <- ["a", "c"]
insert(list, 1, "b")       // list = ["a", "b", "c"]
insert(list, 0, "!")       // list = ["!", "a", "b", "c"]
```

### `remove` input: `container`, `element`
`remove` is a procedure which removes an element from a list or object. If the first parameter passed in is a list,
then the second must be an integer index at which to remove the element from. If the first parameter is an object,
then the second must be the key of a key-value pair to remove.
```
list <- [1, 2, 3, 4]
remove(list, 0)       // list = [2, 3, 4]
remove(list, 2)       // list = [2, 3]
remove(list, 3)       // invalid; index 3 is outside of list bounds

object <- {"a" <- 1, "b" <- 2}
remove(object, "a")      // object = { "b" <- 2 }
remove(object, "c")      // invalid; there is no field "c" in the object
```

### `removeLast` input: `list`
`removeLast` is a procedure which removes the last element from a list.
```
list <- [1, 2, 3]
removeLast(list)      // list = [1, 2]
removeLast(list)      // list = [1]
```

### `type` input: `var`; output: string
`type` is a procedure which outputs a string representation of the input variable's type
```
type(123)    // "number"
type(true)   // "boolean"
type("a")    // "string"
type([])     // "list"
type({})     // "object"
```

### `pow` input: `num`, `power`
`pow` is a procedure which outputs a number equal to the input `number` raised to the input `power`
```
pow(3, 2)          // 9
pow(1, 100)        // 1
pow(1.5, 1.5)      // ~ 1.8371
```

### `log` input: `num`, `base`
`log` is a procedure which outputs a number equal to the logarithm of `number` with the specified `base`
```
log(4, 2)          // 2
log(1000, 10)      // 3
log(27, 3)         // 3
```

### `cos`/`sin`/`tan` input: `angle`
These are procedures which respectively output the cosine/sine/tangent of the specified angle (in radians)

### `acos`/`asin`/`atan` input: `val`
These are procedures which respectively output the inverse cosine/sine/tangent (in radians) of the specified value

### `atan2` input: `y`, `x`
`atan2` is a procedure which outputs the unambiguous inverse tangent (in radians) of the specified values
(first parameter: y value, second: x value)


## Multi-file programs
Pieces of Sudoh programs may be split into multiple source files for further code organization. A `.sud` source file
may specify a list of other `.sud` files in the current directory to import. If this is done, all the procedures in
the other specified source files will then be available to the source file importing them. Note: any code located in
the global scope (not inside a procedure) of the imported files will not be imported. To import from other files, use
the `including` keyword followed by a comma-separated list of files to import on the first relevant (not a comment or
whitespace) line of a source file.
```
// import the procedures in 'import1.sud' and 'hello.sud' into this source file
including import1, hello

including import2   // illegal; all imports must be on the first relevant line of a file
```

The following is an example of a multifile program
```
// file: "importFile.sud"

procedure doStuff
    print("entered doStuff")

procedure squared <- num
    output num * num
```
```
// file: "program.sud"

including importFile   // 'doStuff' and 'squared' now available

doStuff()              // 'entered doStuff' will be printed
print(squared(4))      // '16' will be printed
```
