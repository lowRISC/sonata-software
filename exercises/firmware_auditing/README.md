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

## Part 2 - check only permitted functions have interrupts disabled

The policy for this exercise can be found in the [`interrupt_disables.rego`] file.
You can either follow along as this exercise explains how the policy works, or try writing your own policy with the same behaviour.
The firmware we are auditing is `firmware_auditing_part_2`, defined in [`xmake.lua`][], which contains two compartments based on the C++ files [`disable_interrupts.cc`][] and [`bad_disable_interrupts.cc`][].

[`interrupt_disables.rego`]: ../../exercises/firmware_auditing/part_2/interrupt_disables.rego
[`xmake.lua`]: ../../exercises/firmware_auditing/xmake.lua
[`disable_interrupts.cc`]: ../../exercises/firmware_auditing/part_2/disable_interrupts.cc
[`bad_disable_interrupts.cc`]: ../../exercises/firmware_auditing/part_2/bad_disable_interrupts.cc

As in the last exercise, we first declare an `interrupts` package, to allow us to use `data.interrupts` in our queries.
We then start by defining which functions in which compartments are allowed to run with interrupts disabled.
We do this by using a list, with each item in this list containing the name of the compartment, and a list of the function signatures that can run with interrupts disabled.

We allow two specific functions to run without interrupts in the `disable_interrupts` compartment, and allow none to run in the `bad_disable_interrupts` compartment.
Despite this, if you check the the source files, `not_allowed` is actually running with interrupts disabled.
In a practical scenario, `disable_interrupts` might be a trusted library, where `bad_disable_interrupts` is only allowed to call functions from `disable_interrupts`, and not disable interrupts itself.

We can then also use this list to construct a smaller set containing just the compartments we expect to be present, which will be useful, as the first condition that we want to check is that all (and only) the required compartments are present.
We make a rule `all_required_compartments_present` which checks for this, by comparing the set of all present compartments and the set of required compartments.
Because we are comparing two sets, `==` does not work as expected, and so we take the set intersection (using `&`) and then check its equality.

We then define a helper rule to allow us to retrieve information about all exported symbols, storing the compartment with each export.
This information will be useful for demangling the names of export symbols.
We then define `exports_with_interrupt_status(status)`, which uses a list comprehension to filter for exports with a given interrupt status.
The three possible status values are `enabled`, `disabled` and `inherit`. See section 3.4 of the [CHERIoT Programmer's Guide][] for further information.

[CHERIoT Programmer's Guide]: https://cheriot.org/book/language_extensions.html#_interrupt_state_control

At this stage, we can try and query the names of exports with disabled interrupts, by using the following query:

```
'[x.export.export_symbol | x = data.interrupts.exports_with_interrupt_status("disabled")[_]]'
```

You should see that we get a lot of symbols that look something like `__library_export_disable_interrupts__Z22run_without_interruptsi`.
This function name has been **mangled** during the compilation process, and can no longer be easily checked against our list.
It is not always trivial to manually demangle these symbols.

Luckily for us, the libstdc++ cross-vendor C++ ABI defines a function `abi::__cxa_demangle` to help demangle these names, and `cheriot-audit` wraps and exposes this through the built-in `export_entry_demangle` function.
This function takes the compartment name and export symbol as its two arguments.

> ℹ️ The next rule `patched_export_entry_demangle` is not relevant to this example.
> It simply manually adds support for an additional library export name mangling prefix that is not currently checked for.
> However, it is a useful example of string operations in Rego, as well as another case of rules with multiple definitions.
> We have one rule for export symbols that start with `__library_export_`, converting this to an `__export_` prefix, and otherwise we simply pass the symbol to `export_entry_demangle`.

Now that we have a method to filter for exported symbols with disabled interrupts, and the means to demangle names, we can create a rule to check that a compartment only has the specified disabled interrupts.
When given a compartment, we search for exports with interrupts disabled that have an `"owner"` which corresponds to that compartment.
We then map this through our export entry demangling function, to retrieve the original signatures.
We compare this to the list of required signatures for the compartment - if it matches, then we assign `true`.

We can now easily express our final condition - for every compartment we have provided, it must contain exactly the list of functions with disabled interrupts that we specified.
This uses the `every` keyword from the `futures.keyword` import to perform universal quantification.

Finally, we create a simple Boolean `valid` rule which combines our two condition rules `all_required_compartments_present` and `only_required_disabled_interrupts_present`, and which is defined to output `false` by default if either fail to match.

Now, we can audit the firmware for this exercise by using the following command:
```sh
cheriot-audit --board=cheriot-rtos/sdk/boards/sonata.json \
              --firmware-report=build/cheriot/cheriot/release/firmware_auditing_part_2.json \
              --module=exercises/firmware_auditing/part_2/interrupt_disables.rego \
              --query='data.interrupts.valid'
```

You can see that this returns `false`, because our current firmware does not conform to our policy. You can try changing any of the following:

1. Add `not_allowed()` to the list of allowed functions in `bad_disabled_interrupts` in the Rego policy.
2. Change the `[[cheri::interrupt_state(disabled)]]` tag on the `not_allowed()` function to `enabled` and rebuild.
3. Remove the `[[cheri::interrupt_state(disabled)]]` tag on the `not_allowed()` function entirely and rebuild.
This works because interrupts default to being `enabled`.

Doing any of these three changes above and auditing should now output `true`, as the policy now matches the firmware image.

You can now try individually querying some of the helper rules that we have made, and using them to build your own rules.
For this exercise, we consider both **sufficiency** and **necessity** - there is no way to *allow* a function to have interrupts disabled but not *require* it.
You can try incorporating this addition to extend the exercise further, making an even more powerful and expressive policy.

## Part 3 - audit maximum allocation limits

The policy for this exercise can be found in the [`malloc_check.rego`] file.
You can either follow along as this exercise explains how the policy works, or try writing your own policy with the same behaviour.
The firmware we are auditing is `firmware_auditing_part_3`, defined in [`xmake.lua`][], which contains four compartments based on the C++ files [`malloc1024.cc`][], [`malloc2048.cc`][], [`malloc4096.cc`][] and [`malloc_many.cc`][].
The first 3 files allocate 1024, 2048 and 4096 bytes respectively.
The fourth defines a variety of heap allocation functions with varying quotas to emulate more complex firmware.

[`malloc_check.rego`]: ../../exercises/firmware_auditing/part_3/malloc_check.rego
[`xmake.lua`]: ../../exercises/firmware_auditing/xmake.lua
[`malloc1024.cc`]: ../../exercises/firmware_auditing/part_3/malloc1024.cc
[`malloc2048.cc`]: ../../exercises/firmware_auditing/part_3/malloc2048.cc
[`malloc4096.cc`]: ../../exercises/firmware_auditing/part_3/malloc4096.cc
[`malloc_many.cc`]: ../../exercises/firmware_auditing/part_3/malloc_many.cc

As in the last exercises, we first declare a `malloc_check` package, to allow us to use `data.malloc_check` in our queries.

For the purpose of this exercise, we need a way to check whether a given capability is an *allocator capability* (i.e. a sealed object, sealed by the compartment `alloc`, with the key `MallocKey`).
We also need a way to decode such allocator capabilities, mapping their contents to an allocation quota.

Fortunately, `cheriot-audit` defines two functions that do exactly this! `data.rtos.is_allocator_capability(capability)` and `data.rtos.decode_allocator_capability(capability)` perform this functionality as described.
There is also a helpful built-in rule `data.rtos.all_sealed_capabilities_are_valid` which decodes all allocator capabilities to ensure that they are all valid for auditing.

Using these built-in functions, we define a rule `allocator_capabilities` which filters through the input for allocator capabilities, and augments each with information about their compartment.
This lets us define our first condition `all_allocations_less_than(limit)` as a parameterised function.
This rule ensures that no individual allocator capability is greater than a given limit, ensuring that only a certain amount of memory can be allocated in a single `malloc`.

Next, we can create a rule to extract the list of unique compartments that allocate on the heap.
We can do this using Rego's `contains` keyword and some term-matching syntax to extract the `"owner"` field.
Using this, we now have a construct which we can use to sum all quotas within a given compartment.
By using the built-in `sum` function, `allocator_capabilities`, and `unique_allocator_compartments`, we can define an object that maps from a given compartment to its total allocation quota.

Following from this, we can define our second allocation limiting rule: `all_compartments_allocate_leq(limit)`.
This function checks that no individual compartment can allocate memory greater than a given limit at one time, across all of its allocator capabilities.
This hence ensures that only a certain amount of memory can be used by one compartment at any given time.

For our final allocation limiting rule, we first define a helper rule `total_quota` which sums up the quota of every allocator capability in the firmware.
We use this to construct the final check `total_allocation_leq(limit)`, which ensures that only a certain amount of memory can be used by firmware at any one time.
This can be useful information for auditing firmware running on systems with limited memory resources, or with multiple processes running simultaneously.

As in our other examples, we finish by making a `valid` rule which evaluates to a Boolean, which audits whether our firmware image is valid.
Using our 3 functions, we can easily set custom allocation limits.
For this exercise, we decide that all sealed allocator capabilities must be valid, all individual allocations must be at most 20 KiB, no compartment must allocate more than 40 KiB at once, and the entire firmware must not allocate more than 100 KiB at once.

We can audit our firmware using the following command:
```sh
cheriot-audit --board=cheriot-rtos/sdk/boards/sonata.json \
              --firmware-report=build/cheriot/cheriot/release/firmware_auditing_part_3.json \
              --module=exercises/firmware_auditing/part_3/malloc_check.rego \
              --query='data.malloc_check.valid'
```

In this instance, the output of this test should be `true`, as our defined firmware meets these properties.
You can check this yourself by looking at the source files and the values of the intermediary rules.
To test that the policy is working, you can either change the amount of memory allocated by the firmware (making sure to rebuild), or change the policy itself to enforce lower limits.
For example, changing the line
```
all_allocations_leq(20480)  # 20 KiB
```
to the new line
```
all_allocations_leq(10240)  # 10 KiB
```
should cause the `valid` rule of the policy to evaluate to `false`, because the `malloc_many` compartment contains a capability that permits the allocation of 16 KiB at once, which is greater than our specified limits.

Try playing around with these values to convince yourself that the policy is working as we expect it to.

## Beyond these exercises

There are other pieces of information that the linker exposes, which we do not use in this exercise. For example:
 - You could check that only certain compartments access certain MMIO capabilities.
 - You could check that only specific compartments call permitted exported functions.
 - You could verify that only certain expected functions are being exported from a library.
 - You could integrate this policy into [SBOM](https://en.wikipedia.org/wiki/Software_supply_chain) verification, checking that:
   - Specific files are included.
   - The hash and size of these files matches.
   - The hash of linked third-party libraries matches known values.
   - Verify the switcher/scheduler/allocator, so that we know that the CHERIoT RTOS used is secure.

Other ideas might include writing a policy that combines the above exercises, or integrating it into the `xmake` build system so that a given policy is automatically run when building your firmware image.
