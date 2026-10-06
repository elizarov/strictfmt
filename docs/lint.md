# Linting

`strictfmt` checks configured naming rules before formatting and reports violations.

## Configuration

```yaml
Lint:
  Naming:
    Constants:
      Kinds: Variable
      Constexpr: true
      Case: CamelCase
      Prefix: k
    Objects:
      Kinds: Variable, Binding, Parameter, ParameterPack
      Case: lower_case
    Types:
      Kinds: Class, Struct, Union, Enum, Typedef, TypeAlias
      Case: CamelCase
    PrivateFields:
      Kinds: Field
      Access: Private
      Suffix: '_'
```

For example, these rules accept `kLimit` for a constexpr variable, `item_count` for
an ordinary variable, and `Request` for a struct. A private field must end in `_`;
its casing is unrestricted because the rule specifies no `Case`.

Rule names are arbitrary diagnostic/configuration identifiers. Each rule contains
independent **declaration selectors** and **name constraints**. Rules are evaluated
in configuration order; the first match owns the declaration, including when its
name is exempted. Put specific selectors before general ones. No matching rule
means no naming requirement.

Selectors:

| Key | Values |
| --- | --- |
| `Kinds` (required) | Comma-separated declaration kinds listed below |
| `Scope` | Comma-separated `Global`, `Member`, `Local` |
| `Access` | `Public`, `Protected`, `Private`; matches members only |
| `Const`, `Constexpr`, `Static` | `true` or `false` |

Kinds are `Class`, `Struct`, `Union`, `Enum`, `EnumConstant`, `Namespace`, `Concept`,
`Typedef`, `TypeAlias`, `TypeTemplateParameter`, `ValueTemplateParameter`,
`TemplateTemplateParameter`, `Function`, `Method`, `Variable`, `Field`, `Parameter`,
`ParameterPack`, `Binding`, and `Macro`. `Variable` and `Binding` are separate
kinds: `Binding` selects structured-binding names. Usually, select both with
`Kinds: Variable, Binding` to apply the same naming rule to both. `Field` means a
non-static data member; static data members are `Variable` with `Scope: Member`.
`Global` includes namespace scope.
Unspecified selectors impose no restriction; specified selectors all must match.
Qualifiers describe the declared object, so `const int* p` is not a const pointer,
whereas `int* const p` is. References themselves are not const-qualified.
For methods, `Const` describes the trailing method qualifier, not its return type.

Name constraints:

| Key | Meaning |
| --- | --- |
| `Case` | `lower_case`, `UPPER_CASE`, `CamelCase`, `camelBack`, or `aNy_CasE` |
| `Prefix`, `Suffix` | Required literal affixes, removed before checking casing |
| `IgnoredRegexp` | POSIX extended expression matched against the whole identifier before checking affixes or casing |

`lower_case` accepts `[a-z][a-z0-9_]*`; `UPPER_CASE` accepts `[A-Z][A-Z0-9_]*`;
`CamelCase` accepts `[A-Z][a-zA-Z0-9]*`; `camelBack` accepts `[a-z][a-zA-Z0-9]*`.
After removing affixes, the ordinary casing check rejects leading/trailing `_`.
An omitted `Case` or `aNy_CasE` permits any nonempty remaining spelling. Affixes
must not overlap, and removing them must leave a nonempty name. An omitted or
empty `IgnoredRegexp` exempts nothing. Invalid regexes, unknown lint keys, kinds or
values, duplicate rule names/options, and missing `Kinds` are configuration errors.

The normal [configuration inheritance](config.md#discovery-and-inheritance) applies.
With `Inherit: Parent`, a child overrides individual options of a rule with the same
name, retaining its order and other options. New rules append. An empty affix or
regex clears the inherited value. To disable lint in a subtree:

```yaml
Inherit: Parent
Lint:
  Enabled: false
```

`Enabled` defaults to true, but no naming rules are enabled by default.

## Suppressions

Suppression annotations in line or block comments disable naming diagnostics for
identifiers on selected source lines:

- `NOLINT` suppresses the annotation's line.
- `NOLINTNEXTLINE` suppresses the next line.
- Paired `NOLINTBEGIN` and `NOLINTEND` suppress a region, including the begin line
  but excluding the end line.

Immediately after an annotation, parentheses may contain a comma-separated list
of lint checks to suppress. These are check names, not source identifiers or
configured rule names such as `Objects`. The check `readability-identifier-naming`
selects all configured naming rules. Other check names are ignored by strictfmt,
so a comment can also serve other linters. An empty list suppresses nothing;
omitting the list or including `*` suppresses all checks.

Annotations must be standalone words in comments; text inside string or character
literals has no effect. For a `lower_case` variable rule, all names below are
exempt except `StillChecked`:

```cpp
int ExternalName;  // NOLINT(readability-identifier-naming)
int ImportedName;  // NOLINT
// NOLINTNEXTLINE(readability-identifier-naming, performance-implicit-conversion-in-loop)
int LegacyName;
// NOLINTBEGIN(*)
int FirstGeneratedName;
int SecondGeneratedName;
// NOLINTEND(*)
int StillChecked;  // NOLINT(performance-implicit-conversion-in-loop)
```

## Coverage

Both conditional-compilation branches are checked. Macro definitions check their
names, not parameters or declarations generated by replacement text. Constructors,
destructors, operators, `main`, methods marked `override` (or `final` without an
explicit `virtual` specifier), and qualified redefinitions are not independently
checked against callable naming.

This is a syntax check. It cannot infer qualifiers hidden behind external aliases,
resolve inherited methods without `override`, or inspect macro expansions.
Locally visible function-type aliases are recognized; aliases from other files are not.
Forward type declarations are checked at their definitions. Namespace blocks
containing only forward declarations are skipped, as are identifiers that are
object-like macros defined in the same file. Semantic checks such as implicit
conversions in range loops remain the responsibility of compiler-based tools.

## Declaration ambiguity

Without type information, a parenthesized declaration can name either a function
or an object initialized with expressions:

```cpp
Widget value(argument);
```

If `argument` names a type, this can declare a function. If it names a value, it
can initialize an object. The formatter grammar chooses a syntax interpretation
for layout; the linter must not treat that choice as proof of the declaration's
semantic kind. Ambiguity can also arise with pointers/references and nested
parentheses, such as `Widget value(a & b)`.

The declaration collector records the plausible alternatives. The naming pass
selects a rule independently for each alternative and reports a violation only
when **every alternative rejects the name**. An alternative with no matching rule
imposes no restriction. This keeps classification separate from naming policy;
identifier casing is never used to decide which parse is correct. Conditional
access labels are handled in the same way: after `#endif`, each possible access
level is considered independently. Identifiers inside an ambiguous parameter/argument list are not treated as proven parameter
declarations.

Consequently, naming checks are less strict for ambiguous declarations. For
example, an uppercase name may pass a function rule even when a compiler knows
that the declaration creates a variable. This is an explicit coverage limitation.
A function body, an unambiguous parameter declaration, or other syntactic evidence
can establish a single interpretation. Braced initialization also removes this
particular ambiguity when braces have the intended C++ semantics:

```cpp
Widget value{argument};
```

The checker does not rewrite initialization syntax to resolve ambiguity: changing
parentheses to braces can change overload selection. See also the formatter's
[syntax ambiguity rules](syntax_ambiguities.md).

## Importing from clang-tidy

Convert `readability-identifier-naming` options into `Lint.Naming` rules in
`.cpp-format`, accounting for inherited settings and directory-specific overrides:

1. Group options by category: `<Category>Case`, `<Category>Prefix`,
   `<Category>Suffix`, and `<Category>IgnoredRegexp` become `Case`, `Prefix`,
   `Suffix`, and `IgnoredRegexp` on one rule. Preserve configured exceptions.
2. Express each category with declaration selectors. For example,
   `ConstexprVariable` becomes `Kinds: Variable` with `Constexpr: true`,
   `PrivateMember` becomes `Kinds: Field` with `Access: Private`, and
   `MacroDefinition` becomes `Kinds: Macro`.
3. Preserve category precedence by placing specific rules before general ones.
   Copy only the selected category's constraints; do not merge constraints from
   different categories.

Clang-tidy's variable categories do not check structured-binding names. Use
`Kinds: Variable` to preserve that coverage when converting; add `Binding` to
check binding names as well.

For example, `MemberCase: lower_case` and `PrivateMemberSuffix: '_'` (both under
`readability-identifier-naming`) become:

```yaml
Lint:
  Naming:
    PrivateFields:
      Kinds: Field
      Access: Private
      Suffix: '_'
    OtherFields:
      Kinds: Field
      Case: lower_case
```

Private fields require only the suffix; the general field casing rule does not
also apply. Use `Inherit: Parent` in child configurations to retain shared rules
while applying directory overrides. Existing suppression annotations remain valid;
see [Suppressions](#suppressions). Compare diagnostics, accounting for
[coverage limits](#coverage) and the stricter checks below.

### Stricter naming checks

Strictfmt rejects some names that clang-tidy accepts:

- **Case patterns:** clang-tidy can accept `_` or a non-ASCII name under
  `lower_case`, and `k3dsUrl` under `CamelCase` with `Prefix: k`.
  Strictfmt requires the configured ASCII case pattern.
- **Whole-name regexes:** clang-tidy's `IgnoredRegexp: 'begin|end|hash_value'`
  also exempts `getSpendingCategoriesResponse` because it contains `end`.
  Strictfmt exempts only the three complete names.
- **Nonempty names:** clang-tidy accepts a field named `_` with only
  `Suffix: '_'`; strictfmt requires a nonempty name after removing affixes.

If an exception is intentional, add it explicitly to the affected rule's
`IgnoredRegexp`, for example `_` or `k[0-9][a-zA-Z0-9]*`. Combine it with existing
exceptions using `|`; anchors are unnecessary because the whole name is matched.
