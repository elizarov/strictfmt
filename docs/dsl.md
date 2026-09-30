# Configured DSLs

This document owns configuration and layout rules for domain-specific languages (DSLs) expressed through ordinary C++ calls and operators. Configuration describes call roles; formatting policy is fixed. Configuration discovery and file syntax are specified in [config.md](config.md).

## Stream manipulators

`StreamShift.ConfigurationMethods` lists manipulators that bind to the following shifted value in `<<` and `>>` chains. Keep consecutive configured manipulators and their following value together: do not choose a break between them. Their arguments still use ordinary formatting. An intervening comment or preprocessor directive retains its mandatory break.

Match the complete leading name, including any qualification, with or without a call argument list. For example, `std::hex` matches that name and `std::setw` matches `std::setw(8)`; neither matches a longer identifier. There are no built-in manipulator names. Entries follow the [name-list inheritance rules](config.md#discovery-and-inheritance).

```yaml
StreamShift:
  ConfigurationMethods:
    - std::hex
    - std::setw
```

<!-- .cpp-format
ColumnLimit: 45
StreamShift:
  ConfigurationMethods:
    - std::hex
-->
```cpp
void Print() {
    output
        << "first=" << std::hex << firstValue
        << ", second=" << secondValue;
}
```

Other stream layout rules, including literal pairing, are specified in [format.md](format.md#streams).

## Builder chains

`BuilderChains` contains named profiles for fluent member-call DSLs. Each profile has:

- `EntryCalls`: required list of calls that start this DSL in a member chain.
- `Scopes`: pairs of `Open` and `Close` method names. Different opening methods may share a closing method.
- `BindToNext`: methods that stay attached to the following call, such as a key selector followed by its value.

```yaml
BuilderChains:
  Yson:
    EntryCalls:
      - BuildYsonStringFluently
    Scopes:
      - Open: BeginMap
        Close: EndMap
      - Open: BeginList
        Close: EndList
    BindToNext:
      - Item
  Expressions:
    EntryCalls:
      - Builder
    Scopes:
      - Open: List
        Close: Seal
      - Open: Callable
        Close: Seal
      - Open: Lambda
        Close: Seal
```

### Matching and inheritance

Match exact call names as spelled, including qualification but excluding template arguments. Match member calls by the name after `.` or `->`; match the initial receiver call by its complete name. Thus `Builder` matches `ctx.Builder<T>(pos)`, while `ns::MakeBuilder` matches `ns::MakeBuilder<T>()`. A field access without a call does not match. Names are case-sensitive and do not support patterns or type lookup.

The first matching entry call selects the profile. Keep the receiver through that entry call together, unless the entry is itself a member call with an opening or binding role; then it starts the first builder step. Apply the configured roles to subsequent member calls. Unlisted methods are ordinary builder steps. Calls inside arguments form independent chains. A chain without a matching entry uses ordinary member-call formatting.

Listing an opening or binding method in `EntryCalls` also recognizes fragments such as `builder.BeginMap()` or `builder.Item("key")`. This applies to any receiver with that call name, so use distinctive entry names when unrelated APIs share method names.

Profiles merge by name across configuration inheritance. Their entry calls, scope pairs, and binding methods merge, retaining exact duplicates once. An entry name cannot belong to multiple profiles. A method cannot have conflicting roles, and an opening method cannot name multiple closing methods. Invalid configurations are errors.

### Layout

In a split builder chain, put each step on its own line one continuation level below the receiver. After an opening call, indent its contents one additional level. Put the matching closing call at the opening step's indentation. A nonempty scope forces the chain to split even when it would fit on one line. Consecutive opening and matching closing calls form an empty scope and stay together; empty scopes alone do not force splitting.

Keep a `BindToNext` call attached to the following call, including when that call opens a scope. Apply the ordinary [comment rules](format.md#source-controlled-expansion) and mandatory directive breaks. Call arguments, including nested builders and lambdas, follow ordinary formatting rules.

<!-- .cpp-format
BuilderChains:
  Yson:
    EntryCalls:
      - BuildYsonStringFluently
    Scopes:
      - Open: BeginMap
        Close: EndMap
      - Open: BeginList
        Close: EndList
    BindToNext:
      - Item
-->
```cpp
auto value = BuildYsonStringFluently()
    .BeginMap()
        .Item("name").Value(name)
        .Item("children").BeginList()
            .Item().Value(first)
            .Item().Value(second)
        .EndList()
    .EndMap();
```

Scope matching uses a stack within each chain. If a configured opening or closing call has no matching partner, or pairs are improperly nested, format that chain using ordinary member-call rules. This also accommodates deliberately incomplete builders in tests.

Configure only calls whose role is unambiguous from their name. Overload-dependent roles, such as an opening `With(index)` and a non-opening `With(name)`, require information beyond this configuration. Scopes spanning conditional-compilation alternatives currently use ordinary member-chain formatting.
