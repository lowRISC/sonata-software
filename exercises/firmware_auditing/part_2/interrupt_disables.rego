# Copyright lowRISC Contributors.
# SPDX-License-Identifier: Apache-2.0
package interrupts

import future.keywords

# Note - Any compartment not in this array is not checked by default,
# and so are allowed to have disabled interrupts.
required_disabled_interrupts := [
    {
        "compartment": "disable_interrupts",
        "functions": {"run_without_interrupts(int)", "also_without_interrupts(int*)"}
    }, {
        "compartment": "bad_disable_interrupts",
        "functions": {}
        # Uncomment the below line (and comment the above line) to allow the disallowed
        # function to be run with interrupts disabled
        #"functions": {"not_allowed()"}
    }
]
required_compartments := {x.compartment | x = required_disabled_interrupts[_]}

all_exports := [
    {"owner": owner, "export": export} | export = input.compartments[owner].exports[_]
]

all_required_compartments_present {
    compartments := {name | input.compartments[name]}
    required_compartments == required_compartments & compartments
}

exports_with_interrupt_status(status) := [
    export | export = all_exports[_] ; export["export"]["interrupt_status"] = status
]

patched_export_entry_demangle(compartment, export_symbol) := demangled {
    startswith(export_symbol, "__library_export_")
    modified_export = concat("", ["__export_", substring(export_symbol, 17, -1)])
    demangled := export_entry_demangle(compartment, modified_export)
}
patched_export_entry_demangle(compartment, export_symbol) := demangled {
    not startswith(export_symbol, "__library_export_")
    demangled := export_entry_demangle(compartment, export_symbol)
}

compartment_only_required_disabled_interrupts(compartment) {
    symbols := {
        patched_export_entry_demangle(e["owner"], e["export"]["export_symbol"]) |
        e = exports_with_interrupt_status("disabled")[_] ;
        e["owner"] = compartment["compartment"]
    }
    required := {func | _ = compartment["functions"][func]}
    symbols == required
}

only_required_disabled_interrupts_present {
    every compartment in required_disabled_interrupts {
        compartment_only_required_disabled_interrupts(compartment)
    }
}

default valid := false
valid {
    all_required_compartments_present
    only_required_disabled_interrupts_present
}
