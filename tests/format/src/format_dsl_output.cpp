auto tree = ctx.Builder(pos)
    .List()
        .List(0)
            .Atom(0, "x")
        .Seal()
        .Atom(1, "y")
    .Seal()
    .Build();

auto value = BuildYsonStringFluently()
    .BeginMap()
        .Item("name").Value(name)
        .Item("children").BeginList()
            .Item().Value(first)
            .Item().Value(second)
        .EndList()
    .EndMap();

auto empty = BuildYsonStringFluently().BeginMap().EndMap();

auto emptyChild = BuildYsonStringFluently()
    .BeginMap()
        .Item("empty").BeginList().EndList()
    .EndMap();

auto fragment = other
    .List()
        .Atom(0, "x")
    .Seal()
    .Build();

auto unknown = other.UnconfiguredList().Atom(0, "x").Seal().Build();

auto fields = ctx.Builder.UnconfiguredList().Atom(0, "x").Seal();

auto templated = ctx->Builder<Node>(pos)
    ->Callable("f")
        ->Atom(0, "x")
    ->Seal()
    ->Build();

auto qualified = NYT::BuildYsonFluently(consumer)
    .BeginList()
        .Item().Value(1)
    .EndList();

auto incomplete = ctx.Builder(pos).List().Atom(0, "x").Build();

auto mismatched =
    BuildYsonStringFluently().BeginMap().Item("x").Value(1).EndList();

auto comments = ctx.Builder(pos)
    .List()
        // first child
        .Atom(0, "x")  // first value
        .Atom(1, "y")
    // finish list
    .Seal()
    .Build();

void Print() {
    out
        << "first=" << std::hex << firstLongValue
        << ", second=" << std::setw(8) << secondLongValue;
}

auto actions = BuildYsonStringFluently()
    .BeginMap()
        .Item("x").Do([](auto b) {
            b.Value(1);
            Log();
        })
        .Item("y").Value(2)
    .EndMap();

auto wrapped = ctx.Builder(pos)
    .Callable("f")
        .Atom(
            0,
            MakeText(firstLongArgument, secondLongArgument, thirdLongArgument)
        )
    .Seal()
    .Build();

#define MAKE_TREE(pos)    \
    ctx.Builder(pos)      \
        .List()           \
            .Atom(0, "x") \
        .Seal()           \
        .Build()

void BuildInsideFunction() {
    auto value = ctx.Builder(pos)
        .List()
            .Atom(0, "x")
        .Seal()
        .Build();
    Use(value);
}

auto nested = BuildYsonStringFluently()
    .BeginMap()
        .Item("tree").Value(
            ctx.Builder(pos)
                .List()
                    .Atom(0, "x")
                .Seal()
                .Build()
        )
    .EndMap();

auto Make() {
    return ctx.Builder(pos)
        .List()
            .Atom(0, "x")
        .Seal()
        .Build();
}

auto directive = ctx.Builder(pos)
    .List()
#define CHILD 1
        .Atom(0, CHILD)
#undef CHILD
    .Seal()
    .Build();

auto bindingComment = BuildYsonStringFluently()
    .BeginMap()
        .Item("x")
        // value
        .Value(1)
    .EndMap();

auto commentedRoles = ctx.Builder(pos) /* entry */
    .List() /* scope */
        .Atom(0, "x")
    .Seal()
    .Build();

auto commentedKey = BuildYsonStringFluently()
    .BeginMap()
        .Item("x") /* key */ .Value(1)
    .EndMap();

auto lambda = ctx.Builder(pos)
    .Lambda()
        .Param("x")
        .Apply(expr)
            .With(0, "x")
        .Seal()
    .Seal()
    .Build();

auto entryBinding = fluent
    .Item("map").BeginMap()
        .Item("value").Value(1)
    .EndMap();

auto boundWidth = BuildYsonStringFluently()
    .BeginMap()
        .Item(
            "123456789012345678901234567890123456789012345678901234567890"
        ).Value(value)
    .EndMap();
