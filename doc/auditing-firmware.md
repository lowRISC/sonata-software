# Auditing Firmware

This documentation provides an introduction to CHERIoT Audit, alongside a quick guide on developing auditing policies with Rego.
You can find more comprehensive information about these tools in their relevant documentation:
 - [**CHERIoT Audit**](https://github.com/CHERIoT-Platform/cheriot-audit/blob/main/README.md): the source for `cheriot-audit`.
 - [**Rego OPA Documentation**](https://www.openpolicyagent.org/docs/latest/policy-language/): documentation for Rego, the policy language used by CHERIoT Audit.
 - [**CHERIoT Programmer's Guide**](https://cheriot.org/book/): CHERIoT documentation, which contains some small sections about auditing CHERIoT.

## Introduction

When building firmware, the CHERIoT RTOS linker will produce JSON reports that describe the contents and interactions of each compartment.
CHERIoT Audit is a tool that takes these less-comprehensible JSON reports, and consumes them via a policy language called [Rego][] by evaluating a query to produce an output.

[Rego]: https://www.openpolicyagent.org/docs/latest/policy-language/

Rego is a (mostly) declarative programming language which is similar to other logic programming languages like Datalog and Prolog.
It can be used by firmware integrators to write policies, which can then be verified automatically after linking firmware with CHERIoT Audit.
Making use of effective compartmentalisation alongside CHERIoT Audit can help enhance supply-chain security, enabling safe sharing even if an attacker partially compromises your software.
On top of this, it can simultaneously be used to enforce rules on compartments that you develop, to protect against bugs and misconfigurations.

## Using CHERIoT Audit

When using CHERIoT Audit, you must specify the following three options:
 - `-b`/`--board`: the board description JSON file (`sonata.json`).
 - `-j`/`--firmware-report`: the JSON report emitted by the CHERIoT RTOS linker.
 - `-q`/`--query`: the Rego query to run on the report.

First, make sure you have followed the [getting started guide][] to setup your Nix development environment, and then build the examples using the [standard build flow][].
After building the example software, you can query the exports of the `echo` compartment in the `sonata_demo_everything` report:

[getting started guide]: ../doc/getting-started.md
[standard build flow]: ../doc/exploring-cheriot-rtos.md#build-system

```sh
# Run from the root of the sonata-software repository
cheriot-audit --board=cheriot-rtos/sdk/boards/sonata.json \
              --firmware-report=build/cheriot/cheriot/release/sonata_demo_everything.json \
              --query='input.compartments.echo.exports'
```

This should then output something like this:
```json
[{"export_symbol":"__export_echo__Z11entry_pointv", "exported":true, "interrupt_status":"enabled",
  "kind":"Function", "register_arguments":0, "start_offset":16}]
```
This output tells us that a single entry point is being exported by the `echo` compartment, which is a function that runs with interrupts enabled and takes no register arguments.

## Developing with Rego

The [`vscode-opa`](https://www.openpolicyagent.org/integrations/vscode-opa/) extension for VSCode and the [`zed-rego`][] extension for Zed provide syntax highlighting and a language server for linting functionality.

[`vscode-opa`]: https://www.openpolicyagent.org/integrations/vscode-opa/
[`zed-rego`]: https://github.com/StyraInc/zed-rego

Rego operates using a few key abstractions known as *documents*:
- `input`, the input document, is the firmware report JSON files. There are also other input documents such as the board description.
- A set of modules (also known as *virtual documents*), which process the input documents in order to provide us with views that can then be further interpreted or convey more comprehensible information.
You can write your own modules to encapsulate policy for your specific firmware, or use modules created by others.
For example, `cheriot-audit` supports modules for the RTOS and core compartmentalisation abstractions, such as `data.rtos.decode_allocator_capability`.

These documents are then evaluated over a given Rego query to produce a JSON output.
In order to drive automated auditing compliance or signing decisions, this result will be a single value or some Boolean result representing validity.

> ⚠️ Rego as a language has weakly-typed semantics.
> Running policies via `cheriot-audit` will report only some basic semantic errors.
> For example, accessing a key/attribute that does not exist will not raise an error as you may expect, but will instead result in an **undefined decision**.
> CHERIoT Audit currently represents undefined values using an **empty output**.
>
> It is recommended to create appropriately modular policy code (see below) and test incrementally and often to avoid issues arising.

### Creating modules

Writing a policy within a query string quickly becomes untenable; a better approach involves creating a Rego module and defining functions and predicates that encapsulate complex policy logic in a modular fashion.
These rules can then be easily and programatically invoked via a simple query.
For example, you could make a policy file `policies/example.rego` with the following structure:

```python  # This is Rego, not Python, but we at least get a tiny bit of highlighting
package my_example

import future.keywords

example_function_1(arg1, arg2) {
    ...
}

example_function2() {
    ...
}
```
You might then use `cheriot-audit` via a command like:
```sh
cheriot-audit --board=cheriot-rtos/sdk/boards/sonata.json \
              --firmware-report=build/cheriot/cheriot/release/sonata_demo_everything.json \
              --module=policies/examples.rego \
              --query='data.my_example.example_function1("test", input.compartments)'
```
Where `data.my_example` refers to the `my_example` package declared in your included module.
In this fashion, you can define readable and maintainable policies, and easily audit desired properties with a simple query.

### A quick introduction to Rego

Rego is a policy language designed similarly to Datalog, which is a declarative logic inference programming language.
As such, much of Rego's semantics will be familiar if you already know a logic programming language like Prolog, ASP or Datalog.
Otherwise, it may be unfamiliar and unintuitive otherwise.
If you have no such prior experience, it is highly recommended to read over the [Rego documentation][].

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
*Note here that Rego has whitespace-dependent semantics*.


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
