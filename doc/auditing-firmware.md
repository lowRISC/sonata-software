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

## Setup & Usage

First, make sure to follow the [getting started guide][] to install Nix, enter the CHERIoT development environment, and learn how to build software.

[getting started guide]: ../doc/getting-started.md

You can then acquire `cheriot-audit` as a Nix package with the following command

```sh
# Run from the root of the sonata-software repository
nix shell github:lowrisc/lowrisc-nix#cheriot-audit
```

> ℹ️ If you encounter errors when running this, try running it again outside of the `nix develop` environment.

Next, try building the examples using the [standard build flow](../doc/exploring-cheriot-rtos.md#build-system):

```sh
# Run from the root of the sonata-software repository
xmake -P examples
```

### Using CHERIoT Audit

When using CHERIoT Audit, you must specify the following three options:
 - `-b`/`--board`: the board description JSON file (`sonata.json`).
 - `-j`/`--firmware-report`: the JSON report emitted by the CHERIoT RTOS linker.
 - `-q`/`--query`: the Rego query to run on the report.

For example, after building the example software you can query the exports of the `echo` compartment in the `sonata_demo_everything` report:

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
