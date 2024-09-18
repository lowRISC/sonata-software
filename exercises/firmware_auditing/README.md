<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Firmware Auditing Exercise

First, make sure to go to the [building the exercises][] section to see how the exercises are built.

[building the exercises]: ../README.md#building-the-exercises

Next, go to the [auditing firmware][] documentation section to get a brief introduction to `cheriot-audit` and the Rego policy language.
Part 3 of the [hardware access control] exercise also shows a simple example of using CHERIoT Audit to ensure that compartments do not access MMIO capabilities they should not be accessing.

[auditing firmware]: ../../doc/auditing-firmware.md
[hardware access control]: ../hardware_access_control/README.md#part-3

In this exercise, we use the `cheriot-audit` tool to audit the JSON firmware reports produced by the CHERIoT RTOS linker.
This will let us assert a properties about our firmware images at link-time, guaranteeing desired safety checks.
This exercise explores a set of self-contained policies which audit a variety of properties, to give an idea of what can be achieved using CHERIoT Audit.

For this exercise, when the `xmake.lua` build file is mentioned, we are referring to [`exercises/firmware_auditing/xmake.lua`][].

[`exercises/firmware_auditing/xmake.lua`]: ../../exercises/firmware_auditing/xmake.lua

## A quick introduction to Rego

Rego is a policy language designed similarly to Datalog, which is a declarative logic inference programming language.
As such, much of Rego's semantics will be familiar if you already know a logic programming language like Prolog, ASP or Datalog.
Otherwise, it may be unfamiliar and unintuitive otherwise.
If you have no such prior experience, it is highligy recommend to read over the [Rego documentation][].

[Rego documentation]: https://www.openpolicyagent.org/docs/latest/policy-language/

In documents you create *policies*, which are defined by one or more *rules*.
Rules are in turn each composed of an assignment and a condition.
A rule with no assignments evaluates to `true` by default if its condition is satisfied, and a rule with no condition is automatically assigned.

Rules can be written as `name := val {condition}`, where `val` can be either a Scalar or Composite value.
Composite values are arrays, objects or sets, which are created and accessed using semantics similar to Python, e.g. `rect := {"width": 2, "height": 4}` and `numbers = {1, 7, 19}`.
- You can interpret the rule syntax `a := b {c}` as meaning `a IS b IF c`.
- `:=` is the assignment operator.
It assigns values to locally scoped variables/rules.
- `==` is the comparison operator, and checks for JSON equality.
- `=` is the *unification* operator.
It combines assignment and comparison, with Rego assigning (binding) variables to values that make the comparison true.
This lets you declaratively ask for values that make an expression true.

If the body of a rule will never evaluate to `true`, e.g. `v if "a" = "b"`, then the rule is `undefined`.
This means that any expressions using its value will also be `undefined`.

> ℹ️ There is a notable exception to this rule: the `not` keyword acts as follows.
> If the value is `true`, then `not true` is `false`.
Otherwise, if the value is anything else, *including `undefined`*, then the negation of that value is `true`.

Each rule can have multiple conditions.
Rule bodies, marked by braces, are either delimited by semi-colons, or require you to separate individual expressions with newlines.
Note here that Rego has **whitespace-dependent semantics**.


Variables can be freely used to define rules. For example, `t if { a:= 12; b := 34; b > a}`.
When evaluating rule bodies, OPA searches for any variable bindings that make all of the expressions true - multiple such bindings may exist.
This unification strategy is similar to the one found in [Prolog][], where variables in Rego are existentially quantified by default:
- If we have `a := [1, 2, 3]; a[i] = 3`, then the query is satisfied if there exists such an `i` such that all of the query's expressions are simultaneously satisfied.
In this case, `i=2` (note: arrays are zero-indexed).
- Universal quantification ("for all") can be performed using the syntactic sugar `every` keyword, or by negating the corresponding existentially quantified negated expression. For those familiar with logic syntax, this uses `∀x: p == ¬(∃x : ¬p)`.

[Prolog]: https://www.dai.ed.ac.uk/groups/ssp/bookpages/quickprolog/node12.html

Compound rule bodies can be understood as the logical conjunction (AND) of each individual expression.
If any single condition evaluates to `false`, then so does the whole rule body.
In contrast, any rule with multiple definitions is checked in-order until a matching definition is found.
This can be understood as the logical disjunction (OR) of each individual rule.

Rego supports standard list and set operations, as well as comprehensions in the format
```
{mapped_output | iterator binding ; conditional_filters}
````
where conditional filtering is optional.
For example, consider the list comprehension:
```python  # This is Rego, not python, but it gives us some syntax highlighting
[ { "owner": owner, "capability": data.rtos.decode_allocator_capability(c) } |
c := input.compartments[owner].imports[_] ; data.rtos.is_allocator_capability(c) ]
```
Thich is analogous to the following Python list comprehension:
```python
[{"owner": owner, "capability": data.rtos.decode_allocator_capability(c)}
 for owner, value in input.compartments.items()
 for c in value.imports
 if data.rtos.is_allocator_capability(c)]
```

Objects in Rego can be seen as unordered key-value collections, where any type can be used as a key.
Attributes can be accessed via the `object["attr"]` syntax, or using `object.attr` for string attributes.
Back-ticks can be used to create a raw string, which is useful when writing regular expressions for example.

Rego supports functions with the standard function syntax e.g. `foo() := { ... }` or `bar(arg1, arg2) { ... }`.
These are then invoked like `foo()` or `bar(5, {"x": 3, "y": 4})`. Functions may have many inputs, but only one output.
Because of Rego's evaluation strategy, function arguments can also be constant terms - for example `foo([x, {"bar": y}]) := { ... }`.
Alongside Rego's existential quantification and unification rules, this can be used in a similar manner to pattern matching in higher-level languages.

## Part 1 - check that firmware contains no sealed capabilities

The policy for this exercise can be found in the [`no_sealed_capabilities.rego`][] file.
You can either follow along as this exercise explains how the policy works, or try writing your own policy with the same behaviour.
The firmware we are auditing is `firmware_auditing_part_1`, defined in [`xmake.lua`][], which contains one compartment based on the C++ file [`sealed_capability.cc`][].
This is just a toy example, to show off the auditing functionality.

[`no_sealed_capabilities.rego`]:  ../../exercises/firmware_auditing/part_1/no_sealed_capabilities.rego
[`xmake.lua`]: ../../exercises/firmware_auditing/xmake.lua
[`sealed_capability.cc`]:  ../../exercises/firmware_auditing/part_1/sealed_capability.cc

The first thing that this policy does is declares a `no_seal` package.
This is to allow us to include the file as a module when invoking `cheriot-audit`, so that we can just refer to `data.no_seal` to call our implemented functions.
A very simple function `is_sealed_capability` is then defined, which takes the JSON representing a given `capability`, and checks its `kind` attribute to determine whether it is a sealed capability or not.

Next, we use Rego's functionality to create a rule `no_sealed_capabilities_exist`, which should evaluate to `true` only if no sealed capabilities are used in any of the firmware's compartments.
To do this, we perform a list comprehension, unifying with a wildcard variable to iterate over and filter all imported capabilities of all compartments in the firmware.
We then use the built-in`count` function to ensure that the resulting array is empty.

Skipping to the end of the file, we can then create a simple Boolean `valid` rule which corresponds to whether `no_sealed_capabilities_exist` is defined or not.
To convert the undefined value to a Boolean, we use the `default` keyword, which lets us provide a `false` asignment as a fall-through if no other rule definitions match.

We can run this policy on our example firmware using the following command:

```sh
cheriot-audit --board=cheriot-rtos/sdk/boards/sonata.json \
              --firmware-report=build/cheriot/cheriot/release/firmware_auditing_part_1.json \
              --module=exercises/firmware_auditing/part_1/no_sealed_capabilities.rego \
              --query='data.no_seal.valid'
```

Because `sealed_capability.cc` currently doesn't use any sealed capabilities, this policy should evalute to `true`.

Now, navigate to [`sealed_capability.cc`][] and comment out or remove the line `uint32_t arr[ArrSize];`.
Then, uncomment or add the line `uint32_t *arr = new uint32_t[ArrSize];`.
This will change the array `arr` from being allocated on the stack to the heap.
Rebuilding the exercises (using `xmake -P exercises`) and running the same command again should now cause the policy to evaluate to `false`.

[`sealed_capability.cc`]: ../../exercises/firmware_auditing/part_1/sealed_capability.cc

However, it may not immediately be clear why the policy failed.
When designing such a policy, you can see that it may be helpful to have a rule that lets us inspect the details of any sealed capabilities.
We do this with our final `sealed_capability_info` rule, which constructs ojects storing the contents, key, and compartment of each sealed capability.

> ℹ️ Note the use of unification here.
> We iterate over `input.compartments` with the non-defined variable `owner`, and then map the value that is bound this variable to the `owner` field of our new object.

Now, run the `cheriot-audit` command again, but replace `data.no_seal.valid` with `data.no_seal.sealed_capability_info`.
You should see an output that looks something like this:

```json
[{"compartment":"alloc", "contents":"00100000 00000000 00000000 00000000 00000000 00000000",
  "key":"MallocKey", "owner":"sealed_capability"}]
```

This tells us where the sealed capabilty in our firmware originates - a static sealed object owned by our `sealed_capability` compartment, which is used by the RTOS' allocator for authorising memory allocation.
Try experimenting by adding more functionality to this toy example!
For example, try adding some functionality that might cause the RTOS' scheduler's `InterruptKey` static sealed object to be added.
