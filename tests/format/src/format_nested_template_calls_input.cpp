void NestedTemplateCalls(){Require(ns::Get<Value>(Run(Params(State::kReady))));Compare(ns::Get<Error>(Run(Params(State::kReady))),Error::kInvalid);}
