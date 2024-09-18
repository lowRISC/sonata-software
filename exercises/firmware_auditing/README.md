<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# Firmware Auditing Exercise

First, make sure to go to the [building the exercises][] section to see how the exercises are built.

[building the exercises]: ../README.md#building-the-exercises

You might find it useful to look at the [auditing firmware][] documentation to get a brief introduction to `cheriot-audit` and the Rego policy language.

[auditing firmware]: ../../doc/auditing-firmware.md

In this exercise, we use the `cheriot-audit` tool to audit the JSON firmware reports produced by the CHERIoT RTOS linker.
This will let us assert a properties about our firmware images at link-time, guaranteeing desired safety checks.
This exercise explores a set of self-contained policies which audit a variety of properties, to give an idea of what can be achieved using CHERIoT Audit.

For this exercise, when the `xmake.lua` build file is mentioned, we are referring to [`exercises/firmware_auditing/xmake.lua`][].

[`exercises/firmware_auditing/xmake.lua`]: ../../exercises/firmware_auditing/xmake.lua

## Part 1 - check that firmware contains no sealed capabilities

Policies are written using the *Rego* language, which has syntax that may be unfamiliar.
If you find yourself confused whilst going through this exercise, it might be helpful to read over the [introduction to Rego][] in the documentation.

[introduction to Rego]: ../../doc/auditing-firmware.md#a-quick-introduction-to-rego

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
For example, try creating your own sealed capabilities and check the result.
You might also try investigating the values of the different rules that we've made, and filtering or auditing other properties of the capabilities.
