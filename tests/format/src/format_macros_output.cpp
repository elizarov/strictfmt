// Macro replacements extracted from services and existing golden fixtures.
// Identical definitions are represented once.

// avalon-admin/src/views/v1/features/update_check/post/view.cpp
#define ADP_ADD_TO_DRAFT_DIFF(field)   \
    if (body.field) {                  \
        diff.new_.field = *body.field; \
    }

// bebeacon/src/statistics/statistics.hpp
#define RATE_COUNTER(name)                                                                \
    public:                                                                               \
        void Add##name(size_t count = 1) { name##_ += ::utils::statistics::Rate{count}; } \
        auto Get##name() const { return name##_; }                                        \
                                                                                          \
    private:                                                                              \
        ::utils::statistics::RateCounter name##_{};

// bikes-management/src/utils/utils.hpp
#define API_PATH() bike::management::utils::GetApiPath(__FILE__)

// buysell-ai-chats/src/repository/utils/converter.hpp
#define ADD_PG_CONVERTER(converter)                                                               \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct CppToSystemPg<converter::UserType> : CppToSystemPg<converter::PostgresType> {};        \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<converter::UserType> {                                                           \
        using type = TransformParser<converter::UserType, converter::PostgresType, converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<converter::UserType> {                                                          \
        using type = TransformFormatter<converter::UserType, converter::PostgresType, converter>; \
    };                                                                                            \
                                                                                                  \
    } /* namespace traits */                                                                      \
                                                                                                  \
    } /* namespace storages::postgres::io */

// callcenter-stats/src/models/statistics.hpp
#define AUTO_FIELD(name, ...) std::remove_reference_t<decltype(__VA_ARGS__)> name = (__VA_ARGS__)

// campaigncms-be/tests/requirement.hpp
#define REQUIREMENT(...) static_cast<void>(0)

// candidates/src/script_engine/test/utils.cpp
#define CURRENT_SOURCE_ROOT "taxi/uservices/services/candidates"

// candidates/wasm/sdk/detail.hpp
#define CONTEXT_DETAIL(context, detail)                 \
    do {                                                \
        if ((context).NeedDetails()) {                  \
            [[unlikely]] (context).AddDetail((detail)); \
        }                                               \
    } while (0)

// candidates/wasm/sdk/detail.hpp
#define CONTEXT_DETAIL_FMT(context, ...) CONTEXT_DETAIL((context), std::format(__VA_ARGS__))

// candidates/wasm/sdk/logging/log.hpp
#define LOG(level)                                    \
    level < candidates::sdk::logging::GetLogLevel() ? \
        candidates::sdk::logging::Noop{} : candidates::sdk::logging::LogHelper(level).AsLvalue()

// candidates/wasm/sdk/logging/log.hpp
#define LOG_ERROR() LOG(userver::logging::Level::kError)

// candidates/wasm/sdk/logging/log.hpp
#define LOG_WARNING() LOG(userver::logging::Level::kWarning)

// candidates/wasm/sdk/logging/log.hpp
#define LOG_INFO() LOG(userver::logging::Level::kInfo)

// candidates/wasm/sdk/logging/log.hpp
#define LOG_DEBUG() LOG(userver::logging::Level::kDebug)

// candidates/wasm/sdk/logging/log.hpp
#define LOG_TRACE() LOG(userver::logging::Level::kTrace)

// candidates/wasm/sdk/utils/invariant.hpp
#define INVARIANT(condition, message)                               \
    do {                                                            \
        if (condition) {}                                           \
        else {                                                      \
            interop::Log(userver::logging::Level::kError, message); \
            std::abort();                                           \
        }                                                           \
    } while (0)

// cargo-c2c/src/utils/errors.hpp
#define THROW_SERVER_ERROR_MESSAGE(code, message, log_extra)                              \
    {                                                                                     \
        const logging::LogExtra& log_extra_value = log_extra;                             \
        LOG_ERROR("{} | {}", code, message) << log_extra_value;                           \
        throw ErrorMessageException(ErrorMessageException::Type::kServer, code, message); \
    }

// cargo-c2c/src/utils/errors.hpp
#define THROW_CLIENT_ERROR_MESSAGE(code, message, log_extra)                              \
    {                                                                                     \
        const logging::LogExtra& log_extra_value = log_extra;                             \
        LOG_ERROR("{} | {}", code, message) << log_extra_value;                           \
        throw ErrorMessageException(ErrorMessageException::Type::kClient, code, message); \
    }

// cargo-claims-billing/src/utils/sensitive_data_masking.hpp
#define SENSITIVE_DATA_MASKING_STRINGIZE_AND_COMMA(r, data, elem) \
    ::sensitive_data_masking::PathElementHelper(BOOST_PP_STRINGIZE(elem)),

// cargo-claims-billing/src/utils/sensitive_data_masking.hpp
#define SENSITIVE_DATA_MASKING_UNWRAP_SEQ(s, state, x) (::sensitive_data_masking::UnwrapHelper(state).x)

// cargo-claims-billing/src/utils/sensitive_data_masking.hpp
#define SENSITIVE_DATA_MASKING_MAKE_JSON_PATH_SEQ(prefix, seq)                                             \
    (::std::is_void_v<decltype(BOOST_PP_SEQ_FOLD_LEFT(SENSITIVE_DATA_MASKING_UNWRAP_SEQ, (prefix), seq))>, \
     ::sensitive_data_masking::JsonPath{BOOST_PP_SEQ_FOR_EACH(SENSITIVE_DATA_MASKING_STRINGIZE_AND_COMMA, , seq)})

// cargo-claims-billing/src/utils/sensitive_data_masking.hpp
#define MAKE_JSON_PATH(prefix, ...) \
    SENSITIVE_DATA_MASKING_MAKE_JSON_PATH_SEQ(prefix, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

// cargo-corp/src/views/admin/cargo-corp/client/employees-v1/filter/apply/post/view.cpp
#define LOCALIZE(key) result.title_##key = utils_wb::TranslateForWebConstructor(kCurrentTabPrefix + #key, localizer);

// cargo-finance/src/util/optional.hpp
#define EXTRACT_2(opt, member) ((opt)->*&DecayValueType<decltype(opt)>::value_type::member)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_3(opt, m1, m2) EXTRACT_2(EXTRACT_2(opt, m1), m2)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_4(opt, m1, m2, m3) EXTRACT_2(EXTRACT_3(opt, m1, m2), m3)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_5(opt, m1, m2, m3, m4) EXTRACT_2(EXTRACT_4(opt, m1, m2, m3), m4)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_6(opt, m1, m2, m3, m4, m5) EXTRACT_2(EXTRACT_5(opt, m1, m2, m3, m4), m5)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_7(opt, m1, m2, m3, m4, m5, m6) EXTRACT_2(EXTRACT_6(opt, m1, m2, m3, m4, m5), m6)

// cargo-finance/src/util/optional.hpp
#define GET_8TH_ARG(arg1, arg2, arg3, arg4, arg5, arg6, arg7, N, ...) N

// cargo-finance/src/util/optional.hpp
#define EXTRACT_MACRO_CHOOSER(...) \
    GET_8TH_ARG(__VA_ARGS__, EXTRACT_7, EXTRACT_6, EXTRACT_5, EXTRACT_4, EXTRACT_3, EXTRACT_2)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_MEMBER(opt, ...) EXTRACT_MACRO_CHOOSER(opt, __VA_ARGS__)(ExtractStartTag{}->*(opt), __VA_ARGS__)

// cargo-finance/src/util/optional.hpp
#define EXTRACT_REF(opt, ...) EXTRACT_MACRO_CHOOSER(opt, __VA_ARGS__)(ExtractRefStartTag{}->*(opt), __VA_ARGS__)

// cargo-performer-self-assignment/src/domains/proposal/app/services/filters/utils.hpp
#define NAMED_FIELD(field) ::cargo_performer_self_assignment::proposal::app::filters::NamedField(field, #field)

// cargo-performer-self-assignment/src/helpers/mock_response.hpp
#define MOCK_RESPONSE(dependencies, ResponseBodyType)                                                            \
    const auto handler_path_opt = cargo_performer_self_assignment::helpers::mock_response::ExtractHandlerPath(__FILE__); \
    if (handler_path_opt.has_value()) {                                                                          \
        const auto mock = cargo_performer_self_assignment::helpers::mock_response::GetMockResponse(              \
            dependencies, handler_path_opt.value()                                                               \
        );                                                                                                       \
        if (mock.response_body.has_value()) {                                                                    \
            LOG_WARNING() << "Mock response for " << handler_path_opt.value();                                   \
            return Response200{Parse(mock.response_body.value().extra, formats::parse::To<ResponseBodyType>())}; \
        }                                                                                                        \
    }

// cargo-pricing/src/internal/transform/transform_and_location.hpp
#define CARGO_PRICING_TRANSFORM_AND_LOCATION(transform_name)                                               \
    Output transform_name(Output output, const Input& input);                                              \
    inline cargo_pricing::internal::transform::PathToTransformSourceFile Get##transform_name##Location() { \
        return cargo_pricing::internal::transform::BuildPathToTransformSouceFile(__FILE__);                \
    }

// cargo-pricing/src/internal/transform/transform_and_location.hpp
#define CARGO_PRICING_REGISTER_TRANSFORM(transform_name, version) \
    {(transform_name), (version), Get##transform_name##Location()}

// cargo-pricing/src/repository/express/irepository.hpp
#define CARGO_PRICING_IREADREPOSITORY_INCLUDE(PricerIReadRepository) \
    using PricerIReadRepository::Load;                               \
    using PricerIReadRepository::LoadMany;                           \
    using PricerIReadRepository::TryLoad;                            \
    using PricerIReadRepository::TryLoadMany;

// cargo-pricing/src/repository/express/irepository.hpp
#define CARGO_PRICING_IWRITEREPOSITORY_INCLUDE(PricerIWriteRepository) \
    using PricerIWriteRepository::Save;                                \
    using PricerIWriteRepository::SaveCalcMetadata;

// cargo-pricing/src/repository/express/irepository.hpp
#define CARGO_PRICING_IRWREPOSITORY_INCLUDE(PricerIRepository) \
    CARGO_PRICING_IREADREPOSITORY_INCLUDE(PricerIRepository)   \
    CARGO_PRICING_IWRITEREPOSITORY_INCLUDE(PricerIRepository)

// cargo-sf/src/utils/amo_client_wrapper.cpp
#define AMO_CALL(NAMESPACE, HANDLER, should_set_auth_at_start)                                                       \
    const auto& cluster = deps.pg_cargo_sf->GetCluster();                                                            \
    const auto& amo_secdist = deps.extra.amo_secdist;                                                                \
    const auto& auth_url = cargo_sf::utils::GetAmoAuthUrl(deps);                                                     \
    const auto& amo_domain_url = cargo_sf::utils::GetAmoCompanyDomainUrl(deps, domain);                              \
    const auto& amocrm_cargo_client = deps.extra.amocrm_cargo_component.GetClientFor(amo_domain_url.url);            \
    if (should_set_auth_at_start) {                                                                                  \
        request.authorization = cargo_sf::utils::GetAmoAuthTokenFromDb(cluster, amo_secdist, domain);                \
    }                                                                                                                \
    try {                                                                                                            \
        return amocrm_cargo_client.HANDLER(request);                                                                 \
    } catch (const NAMESPACE::Response401&) {                                                                        \
        LOG_INFO() << "Refreshing auth token due to 401 response";                                                   \
        request.authorization =                                                                                      \
            cargo_sf::utils::GetAmoAutTokenWithRefresh(cluster, amo_secdist, auth_url, amocrm_cargo_client, domain); \
        return amocrm_cargo_client.HANDLER(std::move(request));                                                      \
    }

// cctv-admin/src/views/event_helpers.cpp
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

// client-notify/src/models/consents.cpp
#define TAXI_CONSENT_TYPE_FROM_STRING(name, str) \
    if (tag_str == str) {                        \
        return TaxiConsentType::name;            \
    }

// client-notify/src/models/consents.cpp
#define TAXI_CONSENT_TYPE_TO_STRING(name, str) \
    case TaxiConsentType::name:                \
        return str;

// client-notify/src/models/consents.hpp
#define TAXI_CONSENT_TYPES(X)                                                          \
    X(kAfisha, "afisha")                                                               \
    X(kBerizaryad, "berizaryad")                                                       \
    X(kBuyandsellCis, "buyandsell_cis")                                                \
    X(kCare, "care")                                                                   \
    X(kChargeAndGo, "charge_and_go")                                                   \
    X(kChargersLowBatteryLocalNotification, "chargers-low-battery-local-notification") \
    X(kDelivery, "delivery")                                                           \
    X(kDrive, "drive")                                                                 \
    X(kFuel, "fuel")                                                                   \
    X(kHomeServices, "home_services")                                                  \
    X(kIntercity, "intercity")                                                         \
    X(kMaps, "maps")                                                                   \
    X(kMarket, "market")                                                               \
    X(kMarketing, "marketing")                                                         \
    X(kNavi, "navi")                                                                   \
    X(kNewFeature, "new_feature")                                                      \
    X(kOtherServices, "other_services")                                                \
    X(kPartners, "partners")                                                           \
    X(kPay, "pay")                                                                     \
    X(kPersonalGoals, "personal_goals")                                                \
    X(kPharmacy, "pharmacy")                                                           \
    X(kPlaces, "places")                                                               \
    X(kPlus, "plus")                                                                   \
    X(kPromotions, "promotions")                                                       \
    X(kRecommendedRide, "recommended_ride")                                            \
    X(kRentacar, "rentacar")                                                           \
    X(kRestaurants, "restaurants")                                                     \
    X(kScooters, "scooters")                                                           \
    X(kSellAndBuy, "sell_and_buy")                                                     \
    X(kShops, "shops")                                                                 \
    X(kTransportMsk, "transport_msk")                                                  \
    X(kTransportOther, "transport_other")                                              \
    X(kTravel, "travel")                                                               \
    X(kYangoMarket, "yango_market")                                                    \
    X(kYangoTransport, "yango_transport")

// client-notify/src/models/consents.hpp
#define TAXI_CONSENT_TYPE_ENUM(name, str) name,

// client-notify/src/models/consents.hpp
#define TAXI_CONSENT_TYPE_STR(name, str) str,

// configs-storage/lib/schema_regex_validator/schema.cpp
#define BOOST_REGEX_MAX_STATE_COUNT 1000000

// configs-storage/lib/schema_regex_validator/schema.cpp
#define BOOST_REGEX_MAX_BLOCKS 256

// contractor-insurance/src/models/insurance_data.cpp
#define FIELD(x) \
    FieldWithName { #x, x }

// coupons/src/couponcheck/checks/check_error.cpp
#define COUPONCHECK_TO_STRING_HELPER(value) \
    case CheckExceptionCode::value:         \
        return #value;

// coupons/src/couponcheck/checks/check_list.cpp
#define CHECKER(func) \
    CouponChecker { #func, func }

// coupons/src/models/promocode_series.cpp
#define UPDATE_VALUE(field, field_name)                                              \
    UpdateValue(                                                                     \
        values,                                                                      \
        (current_series) ? std::make_optional(current_series->field) : std::nullopt, \
        modified_series.field,                                                       \
        field_name                                                                   \
    )

// coupons/src/views/admin/promocodes/admin_promocodes.hpp
#define COUPONS_EDIT_FIELDS_LIST                                                         \
    COUPONS_EDIT_FIELD(kSeriesId, "series_id")                                           \
    COUPONS_EDIT_FIELD(kServices, "services")                                            \
    COUPONS_EDIT_FIELD(kFinish, "finish")                                                \
    COUPONS_EDIT_FIELD(kCities, "cities")                                                \
    COUPONS_EDIT_FIELD(kClasses, "classes")                                              \
    COUPONS_EDIT_FIELD(kExternalMeta, "external_meta")                                   \
    COUPONS_EDIT_FIELD(kStart, "start")                                                  \
    COUPONS_EDIT_FIELD(kDescr, "descr")                                                  \
    COUPONS_EDIT_FIELD(kBinRanges, "bin_ranges")                                         \
    COUPONS_EDIT_FIELD(kBankName, "bank_name")                                           \
    COUPONS_EDIT_FIELD(kMinCardExpiration, "min_card_expiration")                        \
    COUPONS_EDIT_FIELD(kUsagePerPromocode, "usage_per_promocode")                        \
    COUPONS_EDIT_FIELD(kUserLimit, "user_limit")                                         \
    COUPONS_EDIT_FIELD(kPaymentMethods, "payment_methods")                               \
    COUPONS_EDIT_FIELD(kCreditcardOnly, "creditcard_only")                               \
    COUPONS_EDIT_FIELD(kFirstUsageByClasses, "first_usage_by_classes")                   \
    COUPONS_EDIT_FIELD(kFirstUsageByPaymentMethods, "first_usage_by_payment_methods")    \
    COUPONS_EDIT_FIELD(kApplications, "applications")                                    \
    COUPONS_EDIT_FIELD(kIsUnique, "is_unique")                                           \
    COUPONS_EDIT_FIELD(kRequiresActivationAfter, "requires_activation_after")            \
    COUPONS_EDIT_FIELD(kValue, "value")                                                  \
    COUPONS_EDIT_FIELD(kIsVolatile, "is_volatile")                                       \
    COUPONS_EDIT_FIELD(kCountry, "country")                                              \
    COUPONS_EDIT_FIELD(kCount, "count")                                                  \
    COUPONS_EDIT_FIELD(kForSupport, "for_support")                                       \
    COUPONS_EDIT_FIELD(kPercent, "percent")                                              \
    COUPONS_EDIT_FIELD(kPercentLimitPerTrip, "percent_limit_per_trip")                   \
    COUPONS_EDIT_FIELD(kDistributionChannels, "distribution_channels")                   \
    COUPONS_EDIT_FIELD(kCommercialInfo, "commercial_info")                               \
    COUPONS_EDIT_FIELD(kImageTag, "image_tag")                                           \
    COUPONS_EDIT_FIELD(kPopupImageUrl, "popup_image_url")                                \
    COUPONS_EDIT_FIELD(kGeoRestrictions, "geo_restrictions")                             \
    COUPONS_EDIT_FIELD(kAdditionalDiscountInfo, "additional_discount_info")              \
    COUPONS_EDIT_FIELD(kAppearanceOverride, "appearance_override")                       \
    COUPONS_EDIT_FIELD(kLogicalServices, "logical_services")                             \
    COUPONS_EDIT_FIELD(kTaxiAntifraudOkRequired, "taxi_antifraud_ok_required")           \
    COUPONS_EDIT_FIELD(kEatsAntifraudOkRequired, "eats_antifraud_ok_required")           \
    COUPONS_EDIT_FIELD(kScootersAntifraudOkRequired, "scooters_antifraud_ok_required")   \
    COUPONS_EDIT_FIELD(kMarketplaceManagerOkRequired, "marketplace_manager_ok_required") \
    COUPONS_EDIT_FIELD(kScootersManagerOkRequired, "scooters_manager_ok_required")       \
    COUPONS_EDIT_FIELD(kFinancierApproveOkRequired, "financier_approve_ok_required")     \
    COUPONS_EDIT_FIELD(kFinancialCenterOkRequired, "financial_center_ok_required")       \
    COUPONS_EDIT_FIELD(kBudget, "budget")                                                \
    COUPONS_EDIT_FIELD(kInitiator, "initiator")                                          \
    COUPONS_EDIT_FIELD(kFinancialCenter, "financial_center")                             \
    COUPONS_EDIT_FIELD(kExtra, "extra")                                                  \
    COUPONS_EDIT_FIELD(kExternalBudget, "external_budget")                               \
    COUPONS_EDIT_FIELD(kFirstLimit, "first_limit")                                       \
    COUPONS_EDIT_FIELD(kAdditionalSeriesIds, "additional_series_ids")                    \
    COUPONS_EDIT_FIELD(kSource, "source")                                                \
    COUPONS_EDIT_FIELD(kCurrency, "currency")                                            \
    COUPONS_EDIT_FIELD(kZones, "zones")                                                  \
    COUPONS_EDIT_FIELD(kSeriesGeneratorYandexUids, "series_generator_yandex_uids")       \
    COUPONS_EDIT_FIELD(kClearText, "clear_text")                                         \
    COUPONS_EDIT_FIELD(kTicket, "ticket")                                                \
    COUPONS_EDIT_FIELD(kTickets, "tickets")                                              \
    COUPONS_EDIT_FIELD(kPromocodesTtlHours, "promocodes_ttl_hours")                      \
    COUPONS_EDIT_FIELD(kAvalonTagsSettings, "avalon_tags_settings")

// coupons/src/views/admin/promocodes/admin_promocodes.hpp
#define COUPONS_EDIT_FIELD(name, value) static constexpr const char* name = value;

// coupons/src/views/admin/promocodes/admin_promocodes.hpp
#define COUPONS_EDIT_FIELD(name, value) +1

// coupons/src/views/admin/promocodes/admin_promocodes.hpp
#define COUPONS_EDIT_FIELD(name, value) value,

// crm-scheduler/src/custom/service_enums.cpp
#define MAKE_DIFINITION_ENUM_TO_STRING(BM, ENUM_NAME) \
    std::string EnumToString(ENUM_NAME enum_value) {  \
        if (BM.right.count(enum_value)) {             \
            return BM.right.at(enum_value);           \
        }                                             \
        return "";                                    \
    }

// crm-scheduler/src/custom/service_enums.cpp
#define MAKE_DIFINITION_GET_ENUM_FROM_STRING(BM, ENUM_NAME)         \
    ENUM_NAME Get##ENUM_NAME##FromString(std::string enum_string) { \
        if (BM.left.count(enum_string)) {                           \
            return BM.left.at(enum_string);                         \
        }                                                           \
        return ENUM_NAME::kUnknown;                                 \
    }

// crm-scheduler/src/db/crm_scheduler_pg_types.hpp
#define REGISTER_TYPE(NAME, TYPE) \
    struct NAME##Tag {};          \
    using NAME = utils::StrongTypedef<NAME##Tag, TYPE>;

// csp-storage/src/data-providers/vehicle-permits/fetchers/istanbul_registry_driver_license_test.cpp
#define APPLY_PROBLEM(cur_problem, code) \
    if (problem != cur_problem) do       \
        {                                \
            code;                        \
    } while (0)

// csp-storage/src/utils/deterministic_serialization_benchmark.cpp
#define LIKELY(x) __builtin_expect(!!(x), 1)

// csp-storage/src/utils/deterministic_serialization_benchmark.cpp
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// csp-storage/src/utils/deterministic_serialization_benchmark.cpp
#define LIKELY(x) (x)

// csp-storage/src/utils/deterministic_serialization_benchmark.cpp
#define UNLIKELY(x) (x)

// csp-storage/src/utils/proto_equals_parity_test.cpp
#define EXPECT_BOTH_EQUAL(msg1, msg2)                                                                        \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
        EXPECT_TRUE(our_result) << "Messages should be equal";                                               \
    } while (0)

// csp-storage/src/utils/proto_equals_parity_test.cpp
#define EXPECT_BOTH_NOT_EQUAL(msg1, msg2)                                                                    \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
        EXPECT_FALSE(our_result) << "Messages should NOT be equal";                                          \
    } while (0)

// csp-storage/src/utils/proto_equals_parity_test.cpp
#define EXPECT_PARITY(msg1, msg2)                                                                            \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
    } while (0)

// custom-page-resolver/src/prepare/market_remix/component.cpp
#define ADD_VALUE(result, proto, field)    \
    if (proto.has_##field()) {             \
        result.field = proto.Get##field(); \
    }

// debts/src/protocol/orderkit/exceptions.hpp
#define COMMIT_ERROR_ENUM_MAP(XX)                                                                           \
    XX(kMulticlassOrderWithoutPricingData, "MULTICLASS_ORDER_WITHOUT_PRICING_DATA")                         \
    XX(kForcedSurgeChanged, "FORCED_SURGE_CHANGED")                                                         \
    XX(kPriceChanged, "PRICE_CHANGED")                                                                      \
    XX(kCantConstructRoute, "CANT_CONSTRUCT_ROUTE")                                                         \
    XX(kRouteOverClosedBorder, "ROUTE_OVER_CLOSED_BORDER")                                                  \
    XX(kOfferNotFound, "OFFER_NOT_FOUND")                                                                   \
    XX(kOrderNotFound, "ORDER_NOT_FOUND")                                                                   \
    XX(kUnknownCard, "UNKNOWN_CARD")                                                                        \
    XX(kBadPaymentMethod, "BAD_PAYMENT_METHOD")                                                             \
    XX(kDebtUser, "DEBT_USER")                                                                              \
    XX(kTooManyConcurrentOrders, "TOO_MANY_CONCURRENT_ORDERS")                                              \
    XX(kCorpGlobalDisabled, "CORP_GLOBAL_DISABLED")                                                         \
    XX(kNotCorpClient, "NOT_CORP_CLIENT")                                                                   \
    XX(kCorpClassDisabled, "CORP_CLASS_DISABLED")                                                           \
    XX(kCorpLimitExceeded, "CORP_LIMIT_EXCEEDED")                                                           \
    XX(kCorpCityDisabled, "CORP_CITY_DISABLED")                                                             \
    XX(kCorpDeactivateThresholdError, "CORP_DEACTIVATE_THRESHOLD_ERROR")                                    \
    XX(kCorpInactiveContractError, "CORP_INACTIVE_CONTRACT_ERROR")                                          \
    XX(kCorpServiceError, "CORP_SERVICE_ERROR")                                                             \
    XX(kWrongRequirements, "WRONG_REQUIREMENTS")                                                            \
    XX(kPaymentTypeCardUnsupported, "PAYMENT_TYPE_CARD_UNSUPPORTED")                                        \
    XX(kPaymentTypeCorpUnsupported, "PAYMENT_TYPE_CORP_UNSUPPORTED")                                        \
    XX(kPaymentTypePersonalWalletUnsupported, "PAYMENT_TYPE_PERSONAL_WALLET_UNSUPPORTED")                   \
    XX(kPaymentTypeCashUnsupported, "PAYMENT_TYPE_CASH_UNSUPPORTED")                                        \
    XX(kPaymentTypeCoopAccountUnsupported, "PAYMENT_TYPE_COOP_ACCOUNT_UNSUPPORTED")                         \
    XX(kPaymentTypeCargocorpUnsupported, "PAYMENT_TYPE_CARGOCORP_UNSUPPORTED")                              \
    XX(kPaymentTypeSbpNotSupported, "PAYMENT_TYPE_SBP_NOT_SUPPORTED")                                       \
    XX(kInvalidPhoneNumber, "INVALID_PHONE_NUMBER")                                                         \
    XX(kPartnerOrderLimitExceeded, "PARTNER_ORDER_LIMIT_EXCEEDED")                                          \
    XX(kFraudDetected, "FRAUD_DETECTED")                                                                    \
    XX(kNeedCardAntifraud, "NEED_CARD_ANTIFRAUD")                                                           \
    XX(kRaceCondition, "RACE_CONDITION")                                                                    \
    XX(kTariffIsRestricted, "TARIFF_IS_RESTRICTED")                                                         \
    XX(kTariffIsUnavailable, "TARIFF_IS_UNAVAILABLE")                                                       \
    XX(kPaymentTypeUnacceptable, "PAYMENT_TYPE_UNACCEPTABLE")                                               \
    XX(kMultiorderDisallowed, "MULTIORDER_DISALLOWED")                                                      \
    XX(kMultiorderTempDisallowed, "MULTIORDER_TEMP_DISALLOWED")                                             \
    XX(kDisabledPaymentTypePersonalWalletIfNoYaPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_YA_PLUS") \
    XX(kDisabledPaymentTypePersonalWalletIfNoCashbackPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS") \
    XX(kCoopAccountUnavailable, "COOP_ACCOUNT_UNAVAILABLE")                                                 \
    XX(kAgentPaymentUnavailable, "AGENT_PAYMENT_UNAVAILABLE")                                               \
    XX(kPersonalWalletInsufficientFunds, "PERSONAL_WALLET_INSUFFICIENT_FUNDS")                              \
    XX(kComplementUnavailable, "COMPLEMENT_UNAVAILABLE")                                                    \
    XX(kComplementPaymentChanged, "COMPLEMENT_PAYMENT_CHANGED")                                             \
    XX(kCorpZoneUnavailable, "CORP_ZONE_UNAVAILABLE")                                                       \
    XX(kCorpCannotOrder, "CORP_CANNOT_ORDER")                                                               \
    XX(kDestinationZoneRestrictionError, "DESTINATION_ZONE_RESTRICTION_ERROR")                              \
    XX(kPaymentTypeMismatchesOffer, "PAYMENT_TYPE_MISMATCHES_OFFER")                                        \
    XX(kCorpPaymentMethodMismatchesOffer, "CORP_PAYMENT_METHOD_MISMATCHES_OFFER")                           \
    XX(kDoorToDoorCommentNotFilled, "DOOR_TO_DOOR_COMMENT_NOT_FILLED")                                      \
    XX(kCashRestrictedByTags, "CASH_RESTRICTED_BY_TAGS")                                                    \
    XX(kTariffzoneNotFound, "TARIFFZONE_NOT_FOUND")                                                         \
    XX(kTariffMultiorderExceeded, "TARIFF_MULTIORDER_EXCEEDED")                                             \
    XX(kNoUserEmail, "NO_USER_EMAIL")                                                                       \
    XX(kDeliveryRestricted, "DELIVERY_RESTRICTED")                                                          \
    XX(kPreorderUnavailable, "PREORDER_UNAVAILABLE")                                                        \
    XX(kMultiorderDisallowedForSpammer, "MULTIORDER_DISALLOWED_FOR_SPAMMER")                                \
    XX(kMaasFlowFailed, "MAAS_FLOW_FAILED")                                                                 \
    XX(kAgentApplicationChanged, "AGENT_APPLICATION_CHANGED")                                               \
    XX(kOrderDraftExpired, "ORDER_DRAFT_EXPIRED")

// debts/src/views/v1/debts/patch/view.cpp
#define ERROR(x) Error{std::string(x)}

// delivery-bpm/src/utils/amo_client_wrapper.cpp
#define AMO_CALL(NAMESPACE, HANDLER, should_set_auth_at_start)                                            \
    const auto& cluster = deps.pg_delivery_bpm->GetCluster();                                             \
    const auto& amo_secdist = deps.extra.amo_secdist;                                                     \
    const auto& auth_url = delivery_bpm::utils::GetAmoAuthUrl(deps);                                      \
    const auto& amo_domain_url =                                                                          \
        delivery_bpm::utils::GetAmoCompanyDomainUrl(deps.config[taxi_config::AMOCRM_CARGO_URL], domain);  \
    const auto& amocrm_cargo_client = deps.extra.amocrm_cargo_component.GetClientFor(amo_domain_url.url); \
    if (should_set_auth_at_start) {                                                                       \
        request.authorization = delivery_bpm::utils::GetAmoAuthTokenFromDb(cluster, amo_secdist, domain); \
    }                                                                                                     \
    try {                                                                                                 \
        return amocrm_cargo_client.HANDLER(request);                                                      \
    } catch (const NAMESPACE::Response401&) {                                                             \
        LOG_INFO() << "Refreshing auth token due to 401 response";                                        \
        request.authorization = delivery_bpm::utils::GetAmoAuthTokenWithRefresh(                          \
            cluster, amo_secdist, auth_url, amocrm_cargo_client, domain                                   \
        );                                                                                                \
        return amocrm_cargo_client.HANDLER(std::move(request));                                           \
    }

// delivery-corp-client-traits/src/country_specifics/converters.cpp
#define INSTANTIATE_BILATERAL_CONVERSION(FUNC, TYPE1, TYPE2) \
    template TYPE1 FUNC(const TYPE2&);                       \
    template TYPE1 FUNC(TYPE2&&);                            \
    template TYPE2 FUNC(const TYPE1&);                       \
    template TYPE2 FUNC(TYPE1&&);

// delivery-corp-client-traits/src/country_specifics/converters.cpp
#define INSTANTIATE_BILATERAL_CONVERSION_HANDLERS_INTERNAL(FUNC, TYPE) \
    INSTANTIATE_BILATERAL_CONVERSION(FUNC, handlers::TYPE, internal_country_specifics::TYPE)

// delivery-courier-orders/libs/result/result/impl/result.hpp
#define ENSURE_RESULT_ACCESS(cond, message, location)     \
    do {                                                  \
        if (!(cond)) {                                    \
            throw (location) + yexception() << (message); \
        }                                                 \
    } while (false)

// delivery-courier-orders/src/utils/bind.hpp
#define BIND(callable, ...) std::bind(callable, std::placeholders::_1, __VA_ARGS__)

// delivery-courier-orders/src/utils/error_injection.hpp
#define INJECT_ERROR_IN_TESTSUITE(name)                                                                                \
    do {                                                                                                               \
        TESTPOINT_CALLBACK(                                                                                            \
            std::string("error_injection::") + name, ::formats::json::Value{}, [](const ::formats::json::Value& doc) { \
                if (doc.IsObject() && !doc["inject_faliure"].IsMissing() && doc["inject_faliure"].As<bool>()) {        \
                    throw std::runtime_error{"injected error"};                                                        \
                }                                                                                                      \
            }                                                                                                          \
        );                                                                                                             \
    } while (false)

// delivery-courier-orders/src/utils/result.hpp
#define DECLARE_RESULT(error, ...)                       \
    enum class error##Error : std::uint8_t{__VA_ARGS__}; \
                                                         \
    using error##Result = result::Result<error##Error>

// delivery-courier-orders/src/utils/result.hpp
#define DECLARE_RESULT_VALUE(value, error, ...)          \
    enum class error##Error : std::uint8_t{__VA_ARGS__}; \
                                                         \
    using error##Result = result::ResultValue<value, error##Error>

// delivery-documents/src/utils/mock_response.hpp
#define MOCK_RESPONSE(dependencies, ResponseBodyType)                                                            \
    const auto handler_path_opt = delivery_documents::utils::mock_response::ExtractHandlerPath(__FILE__);        \
    if (handler_path_opt.has_value()) {                                                                          \
        const auto mock =                                                                                        \
            delivery_documents::utils::mock_response::GetMockResponse(dependencies, handler_path_opt.value());   \
        if (mock.response_body.has_value()) {                                                                    \
            LOG_WARNING() << "Mock response for " << handler_path_opt.value();                                   \
            return Response200{Parse(mock.response_body.value().extra, formats::parse::To<ResponseBodyType>())}; \
        }                                                                                                        \
    }

// delivery-global-integrations/src/integrations/atisu/checks.cpp
#define NAMED_VALUE(field) GetNamedValue(CutPrefix(#field), field)

// driver-communications/src/views/images.cpp
#define cimg_use_png

// driver-orders-builder/src/models/setcar/address.cpp
#define CHECK_AND_LOG(a, b, field, ans) \
    if (a.field != b.field) {           \
        ans.push_back(#field);          \
    }

// driver-orders-builder/src/models/setcar/address.cpp
#define CHECK_AND_LOG_OPT(a, b, field, ans)                             \
    if (a.field != b.field) {                                           \
        std::string log = #field;                                       \
        if (a.field) {                                                  \
            log += fmt::format(" first value is {}", a.field.value());  \
        }                                                               \
        if (b.field) {                                                  \
            log += fmt::format(" second value is {}", b.field.value()); \
        }                                                               \
        ans.push_back(log);                                             \
    }

// driver-regulatory-export/src/third_party/egts/de.h
#define __CODER__

// driver-regulatory-export/src/third_party/egts/de.h
#define MILE \
    1.852  // коэфф мили/километры

// driver-regulatory-export/src/third_party/egts/de.h
#define SIZE_TRACKER_FIELD 16

// driver-regulatory-export/src/third_party/egts/de.h
#define BAD_OBJ (-1)

// driver-regulatory-export/src/third_party/egts/de.h
#define SOCKET_BUF_SIZE (4096)

// driver-regulatory-export/src/third_party/egts/de.h
#define MAX_RECORDS (30)

// driver-regulatory-export/src/third_party/egts/egts.cpp
#define MAX_TERMINALS 1000

// driver-regulatory-export/src/third_party/egts/egts.cpp
#define UTS2010 \
    (1262304000)  // unix timestamp 00:00:00 01.01.2010

// driver-regulatory-export/src/third_party/egts/egts.h
#define __EGTS__

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PT_RESPONSE 0

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PT_APPDATA 1

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PT_SIGNED_APPDATA 2

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_ACK_SERVICE (0)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_AUTH_SERVICE (1)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_TELEDATA_SERVICE (2)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_COMMANDS_SERVICE (4)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FIRMWARE_SERVICE (9)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_ECALL_SERVICE (10)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_RECORD_RESPONSE 0

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_POS_DATA 16

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_EXT_POS_DATA 17

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_AD_SENSORS_DATA 18

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_COUNTERS_DATA 19

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_ACCEL_DATA 20

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_STATE_DATA \
    21  // http://forum.gurtam.com/viewtopic.php?pid=48848#p48848

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_LOOPIN_DATA 22

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_ABS_DIG_SENS_DATA 23

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_ABS_AN_SENS_DATA 24

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_ABS_CNTR_DATA 25

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_ABS_LOOPIN_DATA 26

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_LIQUID_LEVEL_SENSOR 27

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_PASSENGERS_COUNTERS 28

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_DISPATCHER_IDENTITY 5

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_EXT_DATA 44

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_TERM_IDENTITY (1)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_MODULE_DATA (2)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_VEHICLE_DATA (3)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_AUTH_PARAMS (6)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_AUTH_INFO (7)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_SERVICE_INFO (8)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_RESULT_CODE (9)

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_IMEI_LEN 15U

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_IMSI_LEN 16U

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_LNGC_LEN 3U

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_NID_LEN 3U

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_MSISDN_LEN 15U

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_SR_COMMAND_DATA 51

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_DOUT_DATA 0x000B

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_POS_DATA 0x000C

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_SENSORS_DATA 0x000D

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_LIN_DATA 0x000E

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_CIN_DATA 0x000F

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_GET_STATE 0x0010

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_FLEET_ODOM_CLEAR 0x0011

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_OK \
    0  // успешно обработано

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_IN_PROGRESS \
    1  // в процессе обработки

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_UNS_PROTOCOL \
    128  // неподдерживаемый протокол

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_DECRYPT_ERROR \
    129  // ошибка декодирования

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_PROC_DENIED \
    130  // обработка запрещена

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_INC_HEADERFORM \
    131  // неверный формат заголовка

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_INC_DATAFORM \
    132  // неверный формат данных

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_UNS_TYPE \
    133  // неподдерживаемый тип

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_NOTEN_PARAMS \
    134  // неверное количество параметров

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_DBL_PROC \
    135  // попытка повторной обработки

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_PROC_SRC_DENIED \
    136  // обработка данных от источника запрещена

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_HEADERCRC_ERROR \
    137  // ошибка контрольной суммы заголовка

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_DATACRC_ERROR \
    138  // ошибка контрольной суммы данных

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_INVDATALEN \
    139  // некорректная длина данных

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_ROUTE_NFOUND \
    140  // маршрут не найден

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_ROUTE_CLOSED \
    141  // маршрут закрыт

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_ROUTE_DENIED \
    142  // маршрутизация запрещена

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_INVADDR \
    143  // неверный адрес

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_TTLEXPIRED \
    144  // превышено количество ретрансляции данных

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_NO_ACK \
    145  // нет подтверждения

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_OBJ_NFOUND \
    146  // объект не найден

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_EVNT_NFOUND \
    147  // событие не найдено

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_SRVC_NFOUND \
    148  // сервис не найден

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_SRVC_DENIED \
    149  // сервис запрещён

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_SRVC_UNKN \
    150  // неизвестный тип сервиса

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_AUTH_DENIED \
    151  // авторизация запрещена

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_ALREADY_EXISTS \
    152  // объект уже существует

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_ID_NFOUND \
    153  // идентификатор не найден

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_INC_DATETIME \
    154  // неправильная дата и время

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_IO_ERROR \
    155  // ошибка ввода/вывода

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_NO_RES_AVAIL \
    156  // недостаточно ресурсов

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_FAULT \
    157  // внутренний сбой модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_PWR_FLT \
    158  // сбой в работе цепи питания модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_PROC_FLT \
    159  // сбой в работе микроконтроллера модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_SW_FLT \
    160  // сбой в работе программы модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_FW_FLT \
    161  // сбой в работе внутреннего ПО модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_IO_FLT \
    162  // сбой в работе блока ввода/вывода модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_MODULE_MEM_FLT \
    163  // сбой в работе внутренней памяти модуля

// driver-regulatory-export/src/third_party/egts/egts.h
#define EGTS_PC_TEST_FAILED \
    164  // тест не пройден

// driver-status/src/caches/driver_orders_test.cpp
#define NOT_END(it) ASSERT_NE(it, stg.GetEnd())

// driver-status/src/views/compress.cpp
#define IMPLEMENT_COMPRESSION(type, data, size)    \
    switch (type) {                                \
        case Compression::kGzip:                   \
            return gzip::Compress(data, size);     \
        case Compression::kLz4:                    \
            return lz4::Compress(data, size);      \
        case Compression::kNone:                   \
            return std::string(data, data + size); \
    }                                              \
    throw std::runtime_error("Unsupported compression type");

// driver-tags/src/models/db.cpp
#define CHECKED_FIELD_ACCESS(field)              \
    UINVARIANT(field, "Field ##field is empty"); \
    return *field

// driver-tags/src/workers/processors/distlocks_contractors_generator.hpp
#define GEN_WORKER_IMPL(z, n, data)                                                                         \
    class ContractorsPartition##n final : public ContractorsBase {                                          \
    public:                                                                                                 \
        static constexpr const char* kName = "contractors-partitioned-" BOOST_PP_STRINGIZE(n) "-processor"; \
        ContractorsPartition##n(                                                                            \
            const components::ComponentConfig& config,                                                      \
            const components::ComponentContext& context                                                     \
        )                                                                                                   \
            : ContractorsBase(config, context, kName, n)                                                    \
        {}                                                                                                  \
    };

// driver-tags/src/workers/processors/distlocks_contractors_generator.hpp
#define GEN_WORKERS(n) BOOST_PP_REPEAT(n, GEN_WORKER_IMPL, )

// eats-billing-info/src/di/default_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 25

// eats-billing-info/src/di/default_injector.hpp
#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2

// eats-billing-processor/src/di/common.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 20

// eats-business-rules/src/di/default_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 35

// eats-catalog/src/localization/keys.cpp
#define PLATFORM_KEY_FUNC(key_name)                                               \
    const TankerKey& key_name(const eats_shared::ApplicationPlatform& platform) { \
        return IsDCApp(platform) ? kDc##key_name : k##key_name;                   \
    }

// eats-catalog/src/utils/cat_assert.hpp
#define CATASSERT(expr)                                         \
    do {                                                        \
        if (!(expr)) {                                          \
            LOG_ERROR() << "Assertion '" << #expr << "' failed" \
        }                                                       \
    } while (0)

// eats-catalog/src/utils/cat_assert.hpp
#define CATASSERT_MSG(expr, msg)                                            \
    do {                                                                    \
        if (!(expr)) {                                                      \
            LOG_ERROR() << "Assertion '" << #expr << "' failed: " << (msg); \
        }                                                                   \
    } while (0)

// eats-catalog-storage/src/models/extra_values.cpp
#define EXTRA_VALUE(name, type)                                              \
    template <>                                                              \
    std::optional<type> ExtraValues::Get() const { return Get<type>(name); } \
    template <>                                                              \
    void ExtraValues::Set(const type& data) const { Set<type>(name, data); } \
    template <>                                                              \
    void ExtraValues::Delete(ExtraValues::Type<type>) const { DeleteValue(name); }

// eats-checkout-base/src/caches/place_cache.cpp
#define MUST_HAVE_VALUE(field)                                             \
    if (!(item.field).has_value()) {                                       \
        LOG_WARNING() << "place " << item.id << " missing field: " #field; \
        return std::nullopt;                                               \
    }

// eats-checkout-offers/src/caches/place_cache.cpp
#define MUST_HAVE_VALUE(field)                                                  \
    do {                                                                        \
        if (!(field).has_value()) {                                             \
            LOG_WARNING() << "place " << place.id << " missing field: " #field; \
            return false;                                                       \
        }                                                                       \
    } while (0)

// eats-countries/src/models/countries/admin/changeset.cpp
#define ADD_CHANGE_INFO_C(key) utils::AddChangeInfo(builder, data.key, #key)

// eats-countries/src/models/countries/admin/changeset.cpp
#define ADD_CHANGE_INFO_U(key) utils::AddChangeInfo(builder, old_country.key, new_data.key, #key)

// eats-countries/src/models/regions/admin/changeset.cpp
#define ADD_CHANGE_INFO_C(key) utils::AddChangeInfo(builder, region.key, #key)

// eats-countries/src/models/regions/admin/changeset.cpp
#define ADD_CHANGE_INFO_U(key) utils::AddChangeInfo(builder, old_region.key, new_region.key, #key)

// eats-countries/src/models/regions/admin/changeset.cpp
#define ADD_CHANGE_INFO_UO(key) utils::AddChangeInfo(builder, old_region.key, std::make_optional(new_region.key), #key)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_3(opt, m1, m2) extract_2(extract_2(opt, m1), m2)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_4(opt, m1, m2, m3) extract_2(extract_3(opt, m1, m2), m3)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_5(opt, m1, m2, m3, m4) extract_2(extract_4(opt, m1, m2, m3), m4)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_6(opt, m1, m2, m3, m4, m5) extract_2(extract_5(opt, m1, m2, m3, m4), m5)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_7(opt, m1, m2, m3, m4, m5, m6) extract_2(extract_6(opt, m1, m2, m3, m4, m5), m6)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT_MACRO_CHOOSER(...) \
    GET_8TH_ARG(__VA_ARGS__, extract_7, extract_6, extract_5, extract_4, extract_3, extract_2)

// eats-delivery-performers-billing/src/util/optional.hpp
#define EXTRACT(...) EXTRACT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// eats-emergency-communications/src/utils/json/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                            \
    namespace storages::postgres::io {                                                               \
                                                                                                     \
    namespace traits {                                                                               \
                                                                                                     \
    template <>                                                                                      \
    struct Input<codegen_type> {                                                                     \
        using Converter = eats_emergency_communications::utils::json::JsonPgConverter<codegen_type>; \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;       \
    };                                                                                               \
                                                                                                     \
    template <>                                                                                      \
    struct Output<codegen_type> {                                                                    \
        using Converter = eats_emergency_communications::utils::json::JsonPgConverter<codegen_type>; \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                               \
                                                                                                     \
    }                                                                                                \
    template <>                                                                                      \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                   \
                                                                                                     \
    }

// eats-eta/src/utils/redis.hpp
#define REDIS_KEY_PART(x) #x, (x)

// eats-layout-configurator/src/utils/json/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_layout_configurator::utils::json::JsonPgConverter<codegen_type>;   \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_layout_configurator::utils::json::JsonPgConverter<codegen_type>;   \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-layout-constructor/src/agl_execution/functions/lc_localize.cpp
#define T_LC_LOCALIZE_SIGNATURE                                                                          \
    asig::StringType, asig::Optional<asig::IntType>, asig::Optional<asig::ObjectType<>>, asig::Optional< \
        asig::BooleanType                                                                                \
    >, asig::Optional<asig::StringType>, asig::Optional<asig::StringType>

// eats-layout-constructor/src/utils/json/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_layout_constructor::utils::json::JsonPgConverter<codegen_type>;    \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_layout_constructor::utils::json::JsonPgConverter<codegen_type>;    \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-menu-tags/src/utils/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_menu_tags::utils::JsonPgConverter<codegen_type>;                   \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_menu_tags::utils::JsonPgConverter<codegen_type>;                   \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-nomenclature-collector/src/integration/task_creator.cpp
#define ENC_GENERATE_TASK_TRAITS(traits_name, clients_namespace, handle_namespace) \
    struct traits_name {                                                           \
        using Client = clients_namespace::Client;                                  \
        using TaskType = clients_namespace::TaskType;                              \
        using Request = clients_namespace::handle_namespace::Request;              \
        using Response400 = clients_namespace::handle_namespace::Response400;      \
        using Response401 = clients_namespace::handle_namespace::Response401;      \
        using Response403 = clients_namespace::handle_namespace::Response403;      \
        using Response404 = clients_namespace::handle_namespace::Response404;      \
        using Response409 = clients_namespace::handle_namespace::Response409;      \
        using Response500 = clients_namespace::handle_namespace::Response500;      \
    }

// eats-nomenclature-collector/src/integration/task_deleter.cpp
#define ENC_GENERATE_TASK_TRAITS(traits_name, clients_namespace, handle_namespace) \
    struct traits_name {                                                           \
        using Client = clients_namespace::Client;                                  \
        using TaskType = clients_namespace::TaskType;                              \
        using Request = clients_namespace::handle_namespace::Request;              \
        using Response400 = clients_namespace::handle_namespace::Response400;      \
        using Response401 = clients_namespace::handle_namespace::Response401;      \
        using Response403 = clients_namespace::handle_namespace::Response403;      \
        using Response404 = clients_namespace::handle_namespace::Response404;      \
        using Response500 = clients_namespace::handle_namespace::Response500;      \
    }

// eats-order-state/src/db/helpers.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_order_state::helpers::JsonPgConverter<codegen_type>;               \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_order_state::helpers::JsonPgConverter<codegen_type>;               \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-orders-billing/src/di/default_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 15

// eats-orders-info/src/utils/compare_utils.hpp
#define COMPARE_AND_UPDATE_FIELD(service_name, mode, new_value, old_value) \
    eats_orders_info::compare_utils::CompareAndUpdateField(#old_value, service_name, mode, new_value, old_value);

// eats-partner-settings/src/helpers/update_kwarg.hpp
#define UPDATE_KWARG_FROM_REQUEST(KWARGS, CONTEXT, METHOD, REQUEST) \
    {                                                               \
        if (REQUEST.CONTEXT.METHOD().has_value()) {                 \
            KWARGS.Update##METHOD(REQUEST.CONTEXT.METHOD##Value()); \
        }                                                           \
    }

// eats-partner-settings/src/helpers/update_kwarg.hpp
#define UPDATE_KWARG(KWARGS, METHOD, VALUE) \
    {                                       \
        KWARGS.Update##METHOD(VALUE);       \
    }

// eats-partners/src/grpc/service.cpp
#define FILL_SEARCH_PARAM(FIELD, TYPE)                                           \
    if (request.FIELD##_size() != 0) {                                           \
        auto values = request.FIELD();                                           \
        params.FIELD.reserve(values.size());                                     \
        std::transform(                                                          \
            std::make_move_iterator(values.begin()),                             \
            std::make_move_iterator(values.end()),                               \
            std::inserter(params.FIELD, params.FIELD.end()),                     \
            [](auto&& item) { return TYPE{std::forward<decltype(item)>(item)}; } \
        );                                                                       \
    }

// eats-partners/src/views/admin/partners/v2/search/post/view.cpp
#define FILL_SEARCH_PARAM(REQ_FIELD, PARAM_FIELD, TYPE)                  \
    if (request.body.REQ_FIELD.has_value()) {                            \
        params.PARAM_FIELD.insert(TYPE(request.body.REQ_FIELD.value())); \
    }

// eats-partners/src/views/admin/partners/v2/update/check/patch/view.cpp
#define UPDATE_PARTNER_NEW(field) \
    if (request.body.field.has_value()) partner_new.field = request.body.field

// eats-partners/src/views/internal/partners/v2/search/post/view.cpp
#define FILL_SEARCH_PARAM(FIELD, TYPE)                                           \
    if (request.body.FIELD.has_value()) {                                        \
        params.FIELD.reserve(request.body.FIELD->size());                        \
        std::transform(                                                          \
            std::make_move_iterator(request.body.FIELD->begin()),                \
            std::make_move_iterator(request.body.FIELD->end()),                  \
            std::inserter(params.FIELD, params.FIELD.end()),                     \
            [](auto&& item) { return TYPE{std::forward<decltype(item)>(item)}; } \
        );                                                                       \
    }

// eats-payouts-notifications/src/helpers/details_builders/details_template.cpp
#define TO_CONSTRUCTOR_ITEM(return_t, input_t)                                                                    \
    constructor::return_t DetailsTemplateParser::ToConstructorItem(const details_template::input_t& item) const { \
        try {                                                                                                     \
            auto ctor_item = constructor::Parse(item.extra, formats::parse::To<constructor::return_t>{});         \
            return ctor_item;                                                                                     \
        } catch (const std::exception& err) {                                                                     \
            throw DetailsTemplateException(err, #return_t);                                                       \
        }                                                                                                         \
    }

// eats-payouts-notifications/src/helpers/details_builders/details_template.cpp
#define TO_CONSTRUCTOR_ITEM_RAW(return_t, input_t)                                                                   \
    constructor::return_t DetailsTemplateParser::ToConstructorItemRaw(const details_template::input_t& item) const { \
        try {                                                                                                        \
            auto ctor_item = constructor::Parse(item.extra, formats::parse::To<constructor::return_t>{});            \
            return ctor_item;                                                                                        \
        } catch (const std::exception& err) {                                                                        \
            throw DetailsTemplateException(err, #return_t);                                                          \
        }                                                                                                            \
    }

// eats-performer-shifts/src/readers/courier_settings_reader/courier_settings_cache_reader.cpp
#define ADD(field) builder[#field] = field

// eats-place-collections/src/utils/json/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_place_collections::utils::json::JsonPgConverter<codegen_type>;     \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_place_collections::utils::json::JsonPgConverter<codegen_type>;     \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-place-leaders/src/caches/eats_place_info_cache.cpp
#define MUST_HAVE_VALUE(field)                                                      \
    do {                                                                            \
        if (!field.has_value()) {                                                   \
            LOG_ERROR() << "Place " << place.place_id << " missing field: " #field; \
            return false;                                                           \
        }                                                                           \
    } while (0)

// eats-place-leaders/src/utils/jsonb.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_place_leaders::utils::JsonPgConverter<codegen_type>;               \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_place_leaders::utils::JsonPgConverter<codegen_type>;               \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-place-monitoring/src/models/eats_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                                     \
    do {                                                                           \
        if (!field.has_value()) {                                                  \
            LOG_ERROR() << "Place " << place.place_id << " has no field: " #field; \
            return false;                                                          \
        }                                                                          \
    } while (0)

// eats-place-onboarding/src/requesters/extsearch_geo.cpp
#define FILL(KIND_ENUM, FIELD)                       \
    if (component.kind(0) == maps_kind::KIND_ENUM) { \
        result.FIELD = component.name();             \
    }

// eats-place-storage/src/utils/place_data_upserters.hpp
#define INIT_UPSERTER(UPSERTER_NAME)                                                                               \
    class UPSERTER_NAME : public IUpserter {                                                                       \
public:                                                                                                            \
        std::string GetMetricLabel() const override;                                                               \
        bool Upsert(                                                                                               \
            storages::postgres::Transaction& trx, handlers::libraries::eats_place_info::PlaceLogbrokerData&& place \
        ) const override;                                                                                          \
    };

// eats-plus-game/src/caches/eats_catalog_storage_places_cache.cpp
#define MUST_HAVE_VALUE(field)                                                  \
    do {                                                                        \
        if (!field.has_value()) {                                               \
            LOG_WARNING() << "place " << place.id << " missing field: " #field; \
            return false;                                                       \
        }                                                                       \
    } while (0)

// eats-pricing-calculator/src/algorithms/continuous/v1/tests/basic_test.cpp
#define CONTINUOUS_BASIC_TEST Y_CAT(Y_CAT(ContinuousV, EPC_ALGO_VERSION), Test)

// eats-pricing-calculator/src/algorithms/continuous/v1/tests/logic_test.cpp
#define TEST_VERSION Y_CAT(V, EPC_ALGO_VERSION)

// eats-pricing-calculator/src/algorithms/continuous/v1/tests/logic_test.cpp
#define CONTINUOUS_EMPTY_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousEmpty)

// eats-pricing-calculator/src/algorithms/continuous/v1/tests/logic_test.cpp
#define CONTINUOUS_FEES_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousFees)

// eats-pricing-calculator/src/algorithms/continuous/v1/tests/logic_test.cpp
#define CONTINUOUS_SIMPLIFIED_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousSimplified)

// eats-pricing-calculator/src/algorithms/continuous/v1/version.hpp
#define EPC_ALGO_VERSION 1

// eats-pricing-calculator/src/algorithms/continuous/v1/version.hpp
#define EPC_ALGO_VERSION_NS Y_CAT(v, EPC_ALGO_VERSION)

// eats-pricing-calculator/src/algorithms/continuous/v15/version.hpp
#define EPC_ALGO_VERSION 15

// eats-pricing-calculator/src/algorithms/continuous/v16/version.hpp
#define EPC_ALGO_VERSION 16

// eats-pricing-calculator/src/algorithms/continuous/v17/tests/basic_test.cpp
#define CONTINUOUS_SIMPLIFIED_TEST Y_CAT(Y_CAT(ContinuousV, EPC_ALGO_VERSION), SimplifiedTest)

// eats-pricing-calculator/src/algorithms/continuous/v17/version.hpp
#define EPC_ALGO_VERSION 17

// eats-pricing-calculator/src/algorithms/global/logging.hpp
#define ALGO_LOG(level, cfg_level)                                                                    \
    for (bool _algo_log_fire = ::algorithms::global::ShouldLog((level), (cfg_level)); _algo_log_fire; \
         _algo_log_fire = false)                                                                      \
    LOG(level)

// eats-pricing-calculator/src/algorithms/thresholds/v1/tests/basic_test.cpp
#define THRESHOLDS_BASIC_TEST Y_CAT(Y_CAT(ThresholdsV, EPC_ALGO_VERSION), Test)

// eats-pricing-calculator/src/algorithms/thresholds/v1/tests/logic_test.cpp
#define THRESHOLDS_EMPTY_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsEmpty)

// eats-pricing-calculator/src/algorithms/thresholds/v1/tests/logic_test.cpp
#define THRESHOLDS_FEES_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsFees)

// eats-pricing-calculator/src/algorithms/thresholds/v1/tests/logic_test.cpp
#define THRESHOLDS_SIMPLIFIED_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsSimplified)

// eats-pricing-calculator/src/algorithms/thresholds/v21/version.hpp
#define EPC_ALGO_VERSION 21

// eats-pricing-calculator/src/algorithms/thresholds/v22/version.hpp
#define EPC_ALGO_VERSION 22

// eats-pricing-calculator/src/algorithms/thresholds/v23/tests/basic_test.cpp
#define THRESHOLDS_SIMPLIFIED_TEST Y_CAT(Y_CAT(ThresholdsV, EPC_ALGO_VERSION), SimplifiedTest)

// eats-pricing-calculator/src/algorithms/thresholds/v23/version.hpp
#define EPC_ALGO_VERSION 23

// eats-pricing-calculator/src/algorithms/thresholds/v24/version.hpp
#define EPC_ALGO_VERSION 24

// eats-pricing-calculator/src/infra/logging/logger.hpp
#define INFRA_LOG(logger, level) \
    for (bool _infra_log_fire = (logger).ShouldLog(level); _infra_log_fire; _infra_log_fire = false) LOG(level)

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_POSITIVE(PROPERTY)                                                                          \
    if (data.PROPERTY.value_or(0) < 0) {                                                                  \
        LOG_LIMITED_WARNING()                                                                             \
            << log_extra << fmt::format("Negative value: {} = {}", #PROPERTY, data.PROPERTY.value_or(0)); \
        return false;                                                                                     \
    }

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_TOTAL(PROPERTY)                                                   \
    do {                                                                        \
        auto total = data.PROPERTY.value_or(0);                                 \
        auto eats = data.eats_##PROPERTY.value_or(0);                           \
        auto dc = data.dc_##PROPERTY.value_or(0);                               \
        if (std::fabs(eats + dc - total) >= kEps) {                             \
            LOG_LIMITED_WARNING()                                               \
                << log_extra                                                    \
                << fmt::format(                                                 \
                       "Equality violation: {} = {}, eats_{} = {}, dc_{} = {}", \
                       #PROPERTY,                                               \
                       total,                                                   \
                       #PROPERTY,                                               \
                       eats,                                                    \
                       #PROPERTY,                                               \
                       dc                                                       \
                   );                                                           \
            return false;                                                       \
        }                                                                       \
    } while (0)

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_TOTAL_ORDERS(PROPERTY_TOTAL, PROPERTY)                   \
    if (data.PROPERTY_TOTAL.value_or(0) < data.PROPERTY.value_or(0)) { \
        LOG_LIMITED_WARNING()                                          \
            << log_extra                                               \
            << fmt::format(                                            \
                   "{} = {} exceeds {} = {}",                          \
                   #PROPERTY,                                          \
                   data.PROPERTY.value_or(0),                          \
                   #PROPERTY_TOTAL,                                    \
                   data.PROPERTY_TOTAL.value_or(0)                     \
               );                                                      \
        return false;                                                  \
    }

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_MAX_VALUES(PROPERTY, PROPERTY_DELTA, MAX_VALUE)                                                     \
    if (data.PROPERTY.has_value() && data.PROPERTY.value() == MAX_VALUE && data.PROPERTY_DELTA.value_or(0) < 0) { \
        LOG_LIMITED_WARNING()                                                                                     \
            << log_extra                                                                                          \
            << fmt::format(                                                                                       \
                   "{} = {} < 0 while {} = {} is max",                                                            \
                   #PROPERTY_DELTA,                                                                               \
                   data.PROPERTY_DELTA.value_or(0),                                                               \
                   #PROPERTY,                                                                                     \
                   data.PROPERTY.value()                                                                          \
               );                                                                                                 \
        return false;                                                                                             \
    }

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_DELTAS(PROPERTY, PROPERTY_DELTA)                          \
    if (data.PROPERTY.has_value() && data.PROPERTY_DELTA.has_value() && \
        data.PROPERTY.value() < data.PROPERTY_DELTA.value())            \
    {                                                                   \
        LOG_LIMITED_WARNING()                                           \
            << log_extra                                                \
            << fmt::format(                                             \
                   "{} = {} exceeds {} = {} value",                     \
                   #PROPERTY_DELTA,                                     \
                   data.PROPERTY_DELTA.value(),                         \
                   #PROPERTY,                                           \
                   data.PROPERTY.value()                                \
               );                                                       \
        return false;                                                   \
    }

// eats-report-storage/src/models/sync/sync_data_validation.cpp
#define CHECK_TIME_PROPERTIES(PROPERTY)                          \
    if (data.PROPERTY.value_or(kMaxTimeLimit) > kMaxTimeLimit) { \
        LOG_LIMITED_WARNING()                                    \
            << log_extra                                         \
            << fmt::format(                                      \
                   "{} = {} exceeds max time limit = {}",        \
                   #PROPERTY,                                    \
                   data.PROPERTY.value_or(kMaxTimeLimit),        \
                   kMaxTimeLimit                                 \
               );                                                \
        return false;                                            \
    }

// eats-report-storage/src/utils/define_struct.hpp
#define ADD_SUFFIX(suffix, ...) __VA_ARGS__##suffix

// eats-report-storage/src/utils/define_struct.hpp
#define WITH_END(seq) ADD_SUFFIX(_END, seq)

// eats-report-storage/src/utils/define_struct.hpp
#define RECURSIVE(macro, seq) WITH_END(macro seq)

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTE(x, y) \
    x y{};                      \
    DECLARE_ATTRIBUTE_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTE_ODD(x, y) \
    x y{};                          \
    DECLARE_ATTRIBUTE_EVEN

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTE_EVEN(x, y) \
    x y{};                           \
    DECLARE_ATTRIBUTE_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTE_ODD_END

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTE_EVEN_END

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_ATTRIBUTES(attributes) RECURSIVE(DECLARE_ATTRIBUTE, attributes)

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_NAME(_, y) #y ATTRIBUTE_NAME_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_NAME_ODD(_, y) , #y ATTRIBUTE_NAME_EVEN

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_NAME_EVEN(_, y) , #y ATTRIBUTE_NAME_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_NAME_ODD_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_NAME_EVEN_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTES_NAMES(attributes) RECURSIVE(ATTRIBUTE_NAME, attributes)

// eats-report-storage/src/utils/define_struct.hpp
#define DEFINE_STRUCT(name, attributes)                                                           \
    struct name {                                                                                 \
        static std::vector<std::string> _attributes_names() {                                     \
            static const std::vector<std::string> attributes_names{ATTRIBUTES_NAMES(attributes)}; \
            return attributes_names;                                                              \
        }                                                                                         \
        DECLARE_ATTRIBUTES(attributes)                                                            \
    };

// eats-report-storage/src/utils/define_struct.hpp
#define DECLARE_FILL_FUNC(field) \
    [this](const eats_report_storage::types::sync::Value& value) { field = value.As<decltype(field)>(); }

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_FUNC(_, y) {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_FUNC_ODD(_, y) , {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_EVEN

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_FUNC_EVEN(_, y) , {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_FUNC_ODD_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_FUNC_EVEN_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTES_TO_FUNC_MAP(attributes) RECURSIVE(ATTRIBUTE_FUNC, attributes)

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_DEFINITION(_, y) y ATTRIBUTE_DEFINITION_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_DEFINITION_ODD(_, y) , y ATTRIBUTE_DEFINITION_EVEN

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_DEFINITION_EVEN(_, y) , y ATTRIBUTE_DEFINITION_ODD

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_DEFINITION_ODD_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTE_DEFINITION_EVEN_END

// eats-report-storage/src/utils/define_struct.hpp
#define ATTRIBUTES_LIST(attributes) RECURSIVE(ATTRIBUTE_DEFINITION, attributes)

// eats-report-storage/src/utils/define_struct.hpp
#define DEFINE_PARSABLE_STRUCT(name, attributes)                                                                    \
    struct name {                                                                                                   \
private:                                                                                                            \
        const std::unordered_map<std::string, eats_report_storage::utils::FieldFillInterface> fill_ops_;            \
                                                                                                                    \
public:                                                                                                             \
        DECLARE_ATTRIBUTES(attributes)                                                                              \
        const std::unordered_map<std::string, eats_report_storage::utils::FieldFillInterface>& GetFillOps() const { \
            return fill_ops_;                                                                                       \
        }                                                                                                           \
        auto Introspect() { return std::tie(ATTRIBUTES_LIST(attributes)); }                                         \
        name() : fill_ops_({ATTRIBUTES_TO_FUNC_MAP(attributes)}) {}                                                 \
    };                                                                                                              \
    inline name Parse(eats_report_storage::types::sync::Row&& row, ::formats::parse::To<name>) {                    \
        name result;                                                                                                \
        auto fill_ops = result.GetFillOps();                                                                        \
        for (auto& [field, fill_op] : fill_ops) {                                                                   \
            if (row.HasMember(field)) {                                                                             \
                const auto& value = row[field];                                                                     \
                if (value.IsNull()) {                                                                               \
                    throw std::runtime_error("Value mustn't be null. Field: " + field);                             \
                }                                                                                                   \
                fill_op(value);                                                                                     \
            }                                                                                                       \
        }                                                                                                           \
        return result;                                                                                              \
    }

// eats-report-storage/tests/models/sync_validation_test.cpp
#define CHECK_POSITIVE(PROPERTY)                 \
    data.PROPERTY.value() *= -1;                 \
    ASSERT_EQ(sync::IsValidYTData(data), false); \
    data.PROPERTY.value() *= -1;

// eats-report-storage/tests/models/sync_validation_test.cpp
#define CHECK_MAX_VALUES(PROPERTY, PROPERTY_DELTA, MAX_VALUE) \
    old_delta_value = data.PROPERTY_DELTA.value();            \
    old_property_value = data.PROPERTY.value();               \
    data.PROPERTY_DELTA.value() = -1;                         \
    data.PROPERTY.value() = MAX_VALUE;                        \
    ASSERT_EQ(sync::IsValidYTData(data), false);              \
    data.PROPERTY_DELTA.value() = old_delta_value;            \
    data.PROPERTY.value() = old_property_value;

// eats-report-storage/tests/models/sync_validation_test.cpp
#define CHECK_DELTAS(PROPERTY, PROPERTY_DELTA)               \
    old_value = data.PROPERTY_DELTA.value();                 \
    data.PROPERTY_DELTA.value() = data.PROPERTY.value() + 1; \
    ASSERT_EQ(sync::IsValidYTData(data), false);             \
    data.PROPERTY_DELTA.value() = old_value;

// eats-report-storage/tests/models/sync_validation_test.cpp
#define CHECK_TIME_PROPERTIES(PROPERTY)          \
    old_value = data.PROPERTY.value();           \
    data.PROPERTY.value() = 3000;                \
    ASSERT_EQ(sync::IsValidYTData(data), false); \
    data.PROPERTY.value() = old_value;

// eats-restapp-communications/src/caches/places_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                              \
    if (!field.has_value()) {                                               \
        LOG_WARNING() << "Place " << place.id << " missing field: " #field; \
        return false;                                                       \
    }

// eats-restapp-marketing/src/caches/places_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                              \
    if (!place.field.has_value()) {                                         \
        LOG_WARNING() << "place " << place.id << " missing field: " #field; \
        return false;                                                       \
    }

// eats-restapp-mpd/src/caches/places_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                                \
    do {                                                                      \
        if (!field.has_value()) {                                             \
            LOG_ERROR() << "Place " << place.id << " missing field: " #field; \
            return false;                                                     \
        }                                                                     \
    } while (0)

// eats-restapp-mpd/src/models/eats_eaters.cpp
#define CHECK_INFO(field)                                                 \
    if (!field.has_value()) {                                             \
        LOG_ERROR() << "Eater " << eater_id << " missing field: " #field; \
        return std::nullopt;                                              \
    }

// eats-restapp-mpd/src/models/eats_order_state.cpp
#define CHECK_INFO(field)                                                          \
    if (!field.has_value()) {                                                      \
        LOG_ERROR() << "Eater of order " << order_nr << " missing field: " #field; \
        return std::nullopt;                                                       \
    }

// eats-restapp-places/tests/utils/sort_delivery_zones_test.cpp
#define DELIVERY_ZONE_FEATURE(ID, NAME, ENABLED)                                                \
    handlers::DeliveryZoneFeatureV3{                                                            \
        .type = handlers::PropertyTypeFeature::kFeature,                                        \
        .properties = handlers::DeliveryZoneInfoV3{.id = ID, .name = NAME, .enabled = ENABLED}, \
    }

// eats-restapp-support-chat/src/components/eats_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                                       \
    do {                                                                             \
        if (!field.has_value()) {                                                    \
            LOG_ERROR() << fmt::format("Place {} missing field: " #field, place.id); \
            return false;                                                            \
        }                                                                            \
    } while (0)

// eats-seo/src/utils/json.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_seo::utils::JsonPgConverter<codegen_type>;                         \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_seo::utils::JsonPgConverter<codegen_type>;                         \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// eats-user-search-history/src/requesters/eats_eaters/requester_test.cpp
#define UNIMPLEMENTED_METHOD(ReturnType, MethodName, RequestType)                                                     \
    ReturnType                                                                                                        \
        MethodName(const RequestType& /*request*/, const clients::eats_eaters::CommandControl& /*cc*/) const override \
    {                                                                                                                 \
        throw std::logic_error("not implemented");                                                                    \
    }                                                                                                                 \
    ::clients::codegen::ResponseFuture<ReturnType> Async##MethodName(                                                 \
        const RequestType& /*request*/, const clients::eats_eaters::CommandControl& /*cc*/                            \
    ) const override {                                                                                                \
        throw std::logic_error("not implemented");                                                                    \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define NFIELD_FOR_EACH_SERVICE(XX)                                                                  \
    XX(SHOW_UID, ShowUid, ShowUid, ShowUid)                                                          \
    XX(PP, Pp, Pp, Pp)                                                                               \
    XX(BID, Bid, Bid, Bid)                                                                           \
    XX(COST, Cost, BrokeredFee, Cost)                                                                \
    XX(ADVERTISER_TYPE, AdvertiserType, AdvertiserType, AdvertiserType)                              \
    XX(DATASOURCE_ID, DatasourceId, DatasourceId, DatasourceId)                                      \
    XX(GEO_ID, GeoId, UserRegion, Rids)                                                              \
    XX(YANDEX_UID, YandexUid, YandexUid, YandexUid)                                                  \
    XX(UUID, Uuid, Uuid, Uuid)                                                                       \
    XX(PUID, Puid, PassportUid, Puid)                                                                \
    XX(ICOOKIE, Icookie, XYandexICookie, Icookie)                                                    \
    XX(IP, Ip, Ip, Ip)                                                                               \
    XX(GEN_TIME, GenTime, GenTime, GenTime)                                                          \
    XX(REQID, Reqid, ReqId, TraceId)                                                                 \
    XX(POSITION, Position, Position, Position)                                                       \
    XX(ADVERTISEMENT_TYPE, AdvertisementType, AdvertisementType, AdvertisementType)                  \
    XX(ADVERTISEMENT_ID, AdvertisementId, AdvertisementId, AdvertisementId)                          \
    XX(ADVERTISER_COUNTRY_ID, AdvertiserCountryId, AdvertiserCountryId, AdvertiserCountryId)         \
    XX(RP, Rp, CpmRpFee, RpFee)                                                                      \
    XX(ADVERTISER_ID, AdvertiserId, AdvertiserId, AdvertiserId)                                      \
    XX(PLATFORM, Platform, Platform, Platform)                                                       \
    XX(TARGET_PAGE, TargetPage, TargetPage, TargetPage)                                              \
    XX(FORMAT, Format, Format, Format)                                                               \
    XX(EVENT_TYPE, EventType, EventType, EventType)                                                  \
    XX(SERVICE_TYPE, ServiceType, ServiceType, ServiceType)                                          \
    XX(ADVERTISER_SERVICE_TYPE, AdvertiserServiceType, AdvertiserServiceType, AdvertiserServiceType) \
    XX(SHOP_ID, ShopId, ShopId, ShopId)                                                              \
    XX(OFFER_ID, OfferId, OfferId, OfferId)                                                          \
    XX(FEED_ID, FeedId, FeedId, FeedId)                                                              \
    XX(SUPPLIER_ID, SupplierId, SupplierId, SupplierId)                                              \
    XX(STRATEGY_TYPE, StrategyType, StrategyType, StrategyType)                                      \
    XX(MARKETPLACE_REGION, MarketplaceRegion, MarketplaceRegion, MarketplaceRegion)                  \
    XX(MULTIPLE_FEES_CPM, MultipleFeesCpm, MultipleFeesCpm, MultipleFeesCpm)                         \
    XX(MULTIPLE_FEES_CPM_AB, MultipleFeesCpmAb, BrokeredCpmMultipleFees, MultipleFeesCpmAb)          \
    XX(                                                                                              \
        ADVERTISER_COUNTRY_AND_MARKETPLACE_REGION,                                                   \
        AdvertiserCountryAndMarketplaceRegion,                                                       \
        AdvertiserCountryAndMarketplaceRegion,                                                       \
        AdvertiserCountryAndMarketplaceRegion                                                        \
    )                                                                                                \
    XX(COST_GREATER_THAN_BID, CostGreaterThanBid, CostGreaterThanBid, CostGreaterThanBid)

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define ENUM_FIELD(EnumName, Name, Market, Lavka) EnumName,

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define NAME_FIELD(EnumName, Name, Market, Lavka) #Name,

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define LAVKA_FIELD_FUNCTION_PAIR(EnumName, Name, Market, Lavka) {LavkaValidate##Lavka},

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define MARKET_FIELD_FUNCTION_PAIR(EnumName, Name, Market, Lavka) {MarketValidate##Market},

// event-master/extra/metrics/market/fields_validation/fields_validate_functions.hpp
#define COUNT_INVALID_FIELD(Field)                              \
    if (proto.Get##Field() == decltype(proto.Get##Field()){}) { \
        ++invalidCount;                                         \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_EVENT_FIELD_VALIDATE_FUNC(Field)                                                                        \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) {       \
        Y_UNUSED(index);                                                                                              \
        size_t invalidCount = 0;                                                                                      \
        const ::NMarket::NEventMaster::NApi::TLavkaEventFields& proto = NUtils::NLavka::GetLavkaEventFields(request); \
        COUNT_INVALID_FIELD(Field);                                                                                   \
        return invalidCount;                                                                                          \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_DOCLOG_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const NMarket::NEventMaster::NApi::TLavkaDocLogFields& proto =                                          \
            NUtils::NLavka::GetLavkaDocLogFields(request, index);                                               \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_RANGER_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const NMarket::NEventMaster::NApi::TRangerFields& proto = request.RangerData.GetRangerFields(index);    \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_ALWAYS_VALID_FIELD(Field)                                                                         \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                      \
        Y_UNUSED(index);                                                                                        \
        return 0;                                                                                               \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_ALWAYS_INVALID_FIELD(Field)                                                                       \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                      \
        Y_UNUSED(index);                                                                                        \
        return 1;                                                                                               \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_lavka.cpp
#define LAVKA_DOCFACTORS_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const ::NRanger::DocFactors& proto = request.UrlsRequest.GetDocs(index).GetFactors();                   \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_EVENT_FIELD_VALIDATE_ID_FUNC(Field)                                                               \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(index);                                                                                         \
        const TString value = request.UrlsRequest.GetEventFields().GetMarketEventFields().Get##Field();          \
        const bool invalid = value.empty() || value == "undefined" || value == "-";                              \
        return static_cast<size_t>(invalid);                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_EVENT_FIELD_VALIDATE_FUNC(Field)                                                                  \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(index);                                                                                         \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMarketEventFields& proto =                                           \
            request.UrlsRequest.GetEventFields().GetMarketEventFields();                                         \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_IMPRESSION_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TInputDoc& doc = NUtils::NMarket::GetInputDoc(request, index);        \
        const NMarket::NEventMaster::NApi::TMarketImpressionFields& proto =                                      \
            doc.GetDocLogFields().GetMarketDocLogFields().GetImpressionFields();                                 \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_DOCLOG_FIELD_VALIDATE_FUNC(Field)                                                                       \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) {       \
        size_t invalidCount = 0;                                                                                       \
        const NMarket::NEventMaster::NApi::TInputDoc& doc = NUtils::NMarket::GetInputDoc(request, index);              \
        const NMarket::NEventMaster::NApi::TMarketDocLogFields& proto = doc.GetDocLogFields().GetMarketDocLogFields(); \
        COUNT_INVALID_FIELD(Field);                                                                                    \
        return invalidCount;                                                                                           \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_MADV_LOGFIELD_VALIDATE_FUNC(Field)                                                                \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMadvCommonLogFields& proto =                                         \
            NUtils::NMarket::GetMadvCommonLogFields(request, index);                                             \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_SALE_OFFER_VIEW_ZERO_FEES_SUM_FIELD_VALIDATE_FUNC(Field)                                          \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        const NMarket::NEventMaster::NApi::TMarketSaleOfferView& proto =                                         \
            NUtils::NMarket::GetMarketSaleOfferView(request, index);                                             \
        const TString& feesString = static_cast<TString>(proto.Get##Field());                                    \
        return ValidateFeesString(feesString);                                                                   \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_SALE_OFFER_VIEW_FIELD_VALIDATE_FUNC(Field)                                                        \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMarketSaleOfferView& proto =                                         \
            NUtils::NMarket::GetMarketSaleOfferView(request, index);                                             \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_RANGER_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TRangerFields& proto = request.RangerData.GetRangerFields(index);     \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_ALWAYS_VALID_FIELD(Field)                                                                         \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                       \
        Y_UNUSED(index);                                                                                         \
        return 0;                                                                                                \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_ALWAYS_INVALID_FIELD(Field)                                                                       \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                       \
        Y_UNUSED(index);                                                                                         \
        return 1;                                                                                                \
    }

// event-master/extra/metrics/market/fields_validation/fields_validate_functions_market.cpp
#define MARKET_DOCFACTORS_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const ::NRanger::DocFactors& proto = NUtils::NMarket::GetInputDoc(request, index).GetFactors();          \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

// exp3-configs-anomalies/src/anomalies/detectors/base/detectors_registry_with_data.hpp
#define REGISTER_DETECTOR(TYPE)                                                           \
    namespace {                                                                           \
                                                                                          \
    struct TYPE##Registrar {                                                              \
        TYPE##Registrar() {                                                               \
            anomalies::detectors::DetectorsRegistry::Instance().Register(TYPE::kType, []( \
                models::enums::DetectorType det_type,                                     \
                const std::string& anomaly_code,                                          \
                const anomalies::detectors::DetectionDeps& deps                           \
            ) {                                                                           \
                return std::make_unique<TYPE>(det_type, anomaly_code, deps);              \
            });                                                                           \
        }                                                                                 \
    };                                                                                    \
                                                                                          \
    static TYPE##Registrar global_##TYPE##_registrar;                                     \
                                                                                          \
    }  // namespace

// fintech-offerinfo/src/utils/log/logger.hpp
#define LOG_CUSTOM_DEBUG() LOG(utils::logger::GetCtx().logging_level)

// fiscal-platform/src/domain/utils/validator.hpp
#define GET_VALUE_OR_THROW(obj, field) GetValueOrThrow(obj, &std::decay_t<decltype(obj)>::field, #field)

// fts-taxi-integration-mock/src/internal/aggregated_data_parser.cpp
#define READ_VALUE_TOKEN_TO_VAR_AND_MOVE(token_name)          \
    std::string_view token_name##_token = lexer.next_token(); \
    if (                                                      \
        token_name##_token == "," ||                          \
        token_name##_token == ";" ||                          \
        token_name##_token == "=" ||                          \
        token_name##_token.empty()                            \
    ) {                                                       \
        return std::nullopt;                                  \
    }                                                         \
    lexer.move_forward();

// fts-taxi-integration-mock/src/internal/aggregated_data_parser.cpp
#define SKIP_EXPECTED_TOKEN(expected_value)           \
    {                                                 \
        std::string_view sep = lexer.next_token();    \
        if (sep != expected_value) {                  \
            LOG_INFO()                                \
                << "ParseAggregatedData. Expected "   \
                   "'" #expected_value "', but got '" \
                << std::string(sep) << "'";           \
            return std::nullopt;                      \
        }                                             \
        lexer.move_forward();                         \
    }

// fts-taxi-integration-mock/src/internal/aggregated_data_parser.cpp
#define CONVERT_TOKEN(token_name, to_type, assign_to_var, assign_expr) \
    {                                                                  \
        to_type converted;                                             \
        try {                                                          \
            converted = std::stod(std::string(token_name##_token));    \
            assign_to_var = assign_expr;                               \
        } catch (const std::exception& ex) {                           \
            LOG_INFO()                                                 \
                << "ParseAggregatedData. Exception was thrown during " \
                   "conversion '" #token_name "_token': "              \
                << ex.what();                                          \
            return std::nullopt;                                       \
        }                                                              \
    }

// fts-taxi-integration-mock/src/internal/aggregated_data_parser.cpp
#define CONVERT_TOKEN_IF_NOT_NULL(token_name, to_type, assign_to_var, assign_expr) \
    {                                                                              \
        if (token_name##_token != "null") {                                        \
            CONVERT_TOKEN(token_name, to_type, assign_to_var, assign_expr);        \
        }                                                                          \
    }

// grocery-cart/src/helpers/new_subitems_calc_log.hpp
#define NEW_SUBITEMS_CALC_LOG(X) LOG_INFO() << "NEW_SUBITEMS_CALC " #X ": [" << X << "]"

// grocery-cart/src/helpers/pricing_utils.cpp
#define LOG_PRICING(X) LOG_INFO() << "[PRICING] " #X ": [" << X << "]"

// grocery-menu/src/helpers/detailed_log.hpp
#define LOG_INFO_OPT(enabled, ...) LOG(enabled ? logging::Level::kInfo : logging::Level::kNone, __VA_ARGS__)

// grocery-menu/src/helpers/detailed_log.hpp
#define LOG_WARNING_OPT(enabled, ...) LOG(enabled ? logging::Level::kWarning : logging::Level::kNone, __VA_ARGS__)

// grocery-performer-watcher/src/components/events_writer.cpp
#define CHECK_NOT_NULLOPT(x) CheckNotNullopt((x), #x)

// grocery-retail/src/caches/zones_cache.cpp
#define CHECK_FIELD(field)                                                                            \
    do {                                                                                              \
        if (!item.field.has_value()) {                                                                \
            LOG_DEBUG("Zone {} is missing field '{}'. Will not add zone to cache.", item.id, #field); \
            return std::nullopt;                                                                      \
        }                                                                                             \
    } while (0)

// heatmap-sample-storage/src/internal/calc_js_surge_cells/settings.cpp
#define GET_PARAM(param_name) ((override && override->param_name) ? *override->param_name : settings.param_name)

// heatmap-sample-storage/src/internal/calc_js_surge_cells/settings.cpp
#define INITIALIZE(param_name) .param_name = GET_PARAM(param_name)

// insurance-agent/src/components/statistics_limiter.hpp
#define STATISTICS_IMPL

// insurance-commune-gateway/src/common/types_fwd.hpp
#define CARING_BASIC_TYPES_HPP_SILENCE_DEPRECATION

// insurance-data/src/clients/datum.hpp
#define CLIENTS_DATUM_IMPL

// integration-api-legacy/src/protocol/common/decoupling.hpp
#define STAGES_ENUM_MAP(XX)                   \
    XX(CalculatingOffer, "calculating_offer") \
    XX(Ordercommit, "ordercommit")

// integration-api-legacy/src/protocol/common/decoupling.hpp
#define ERROR_REASON_ENUM_MAP(XX)                    \
    XX(GetCorpTariffFail, "get_corp_tarif_fail")     \
    XX(PluginInternalError, "plugin_internal_error") \
    XX(NoTariffPrice, "no_tariff_price")             \
    XX(UnknownError, "unknown_error")

// integration-api-legacy/src/protocol/common/plugins/exceptions.hpp
#define PLUGIN_RETHROW(exc) throw common::plugins::RethrowableException(std::make_exception_ptr(exc))

// integration-api-legacy/src/protocol/common/preorder/unavailable_reason.hpp
#define UNAVAILABLE_REASONS_ENUM_MAP(XX)                                           \
    XX(PreorderUnavailableForDue, "preorder_unavailable_for_due")                  \
    XX(PreorderUnavailableForPaymentType, "preorder_unavailable_for_payment_type") \
    XX(PreorderUnavailableForTariff, "preorder_unavailable_for_tariff")            \
    XX(PreorderUnavailableForRequirements, "preorder_unavailable_for_requirements")

// integration-api-legacy/src/protocol/config/billing_component_config.hpp
#define BILLING_CLEANUP_CONTRACT_ON_AUTOREORDER_ENUM_MAP(XX) \
    XX(kDisabled, "disabled")                                \
    XX(kJustLogging, "just_logging")                         \
    XX(kEnabled, "enabled")

// integration-api-legacy/src/protocol/config/multiclass_config.cpp
#define INIT_MEMBER(name_) name_(boost::to_upper_copy(std::string("MULTICLASS_" #name_)), docs_map)

// integration-api-legacy/src/protocol/config/multiorder_config.cpp
#define INIT_MEMBER(name_) name_(boost::to_upper_copy(std::string("MULTIORDER_" #name_)), docs_map)

// integration-api-legacy/src/protocol/external/cost_description.hpp
#define ESTIMATE_COST(XX) XX(MINIMAL_COST, "MINIMAL_COST")

// integration-api-legacy/src/protocol/external/estimate/estimate_enums.hpp
#define ESTIMATE_ENUM_MAP(XX)                        \
    XX(CANT_CONSTRUCT_ROUTE, "CANT_CONSTRUCT_ROUTE") \
    XX(CURFEW, "CURFEW")

// integration-api-legacy/src/protocol/external/estimate/plugins/context.hpp
#define HOOK_ENUM_MAP(XX)                \
    XX(BaseCalcStart, "base_calc_start") \
    XX(BaseCalcEnd, "base_calc_end")

// integration-api-legacy/src/protocol/orderkit/commit/plugins/context.hpp
#define HOOK_ENUM_MAP(XX) XX(PendingEnd, "pending_end")

// integration-api-legacy/src/protocol/orderkit/exceptions.hpp
#define COMMIT_ERROR_ENUM_MAP(XX)                                                                                  \
    XX(MULTICLASS_ORDER_WITHOUT_PRICING_DATA, "MULTICLASS_ORDER_WITHOUT_PRICING_DATA")                             \
    XX(FORCED_SURGE_CHANGED, "FORCED_SURGE_CHANGED")                                                               \
    XX(PRICE_CHANGED, "PRICE_CHANGED")                                                                             \
    XX(CANT_CONSTRUCT_ROUTE, "CANT_CONSTRUCT_ROUTE")                                                               \
    XX(ROUTE_OVER_CLOSED_BORDER, "ROUTE_OVER_CLOSED_BORDER")                                                       \
    XX(OFFER_NOT_FOUND, "OFFER_NOT_FOUND")                                                                         \
    XX(ORDER_NOT_FOUND, "ORDER_NOT_FOUND")                                                                         \
    XX(UNKNOWN_CARD, "UNKNOWN_CARD")                                                                               \
    XX(BAD_PAYMENT_METHOD, "BAD_PAYMENT_METHOD")                                                                   \
    XX(DEBT_USER, "DEBT_USER")                                                                                     \
    XX(TOO_MANY_CONCURRENT_ORDERS, "TOO_MANY_CONCURRENT_ORDERS")                                                   \
    XX(CORP_GLOBAL_DISABLED, "CORP_GLOBAL_DISABLED")                                                               \
    XX(NOT_CORP_CLIENT, "NOT_CORP_CLIENT")                                                                         \
    XX(CORP_CLASS_DISABLED, "CORP_CLASS_DISABLED")                                                                 \
    XX(CORP_LIMIT_EXCEEDED, "CORP_LIMIT_EXCEEDED")                                                                 \
    XX(CORP_CITY_DISABLED, "CORP_CITY_DISABLED")                                                                   \
    XX(CORP_DEACTIVATE_THRESHOLD_ERROR, "CORP_DEACTIVATE_THRESHOLD_ERROR")                                         \
    XX(CORP_INACTIVE_CONTRACT_ERROR, "CORP_INACTIVE_CONTRACT_ERROR")                                               \
    XX(CORP_SERVICE_ERROR, "CORP_SERVICE_ERROR")                                                                   \
    XX(WRONG_REQUIREMENTS, "WRONG_REQUIREMENTS")                                                                   \
    XX(PAYMENT_TYPE_CARD_UNSUPPORTED, "PAYMENT_TYPE_CARD_UNSUPPORTED")                                             \
    XX(PAYMENT_TYPE_CORP_UNSUPPORTED, "PAYMENT_TYPE_CORP_UNSUPPORTED")                                             \
    XX(PAYMENT_TYPE_PERSONAL_WALLET_UNSUPPORTED, "PAYMENT_TYPE_PERSONAL_WALLET_UNSUPPORTED")                       \
    XX(PAYMENT_TYPE_CASH_UNSUPPORTED, "PAYMENT_TYPE_CASH_UNSUPPORTED")                                             \
    XX(PAYMENT_TYPE_COOP_ACCOUNT_UNSUPPORTED, "PAYMENT_TYPE_COOP_ACCOUNT_UNSUPPORTED")                             \
    XX(PAYMENT_TYPE_CARGOCORP_UNSUPPORTED, "PAYMENT_TYPE_CARGOCORP_UNSUPPORTED")                                   \
    XX(PAYMENT_TYPE_SBP_NOT_SUPPORTED, "PAYMENT_TYPE_SBP_NOT_SUPPORTED")                                           \
    XX(INVALID_PHONE_NUMBER, "INVALID_PHONE_NUMBER")                                                               \
    XX(PARTNER_ORDER_LIMIT_EXCEEDED, "PARTNER_ORDER_LIMIT_EXCEEDED")                                               \
    XX(FRAUD_DETECTED, "FRAUD_DETECTED")                                                                           \
    XX(NEED_CARD_ANTIFRAUD, "NEED_CARD_ANTIFRAUD")                                                                 \
    XX(RACE_CONDITION, "RACE_CONDITION")                                                                           \
    XX(TARIFF_IS_RESTRICTED, "TARIFF_IS_RESTRICTED")                                                               \
    XX(TARIFF_IS_UNAVAILABLE, "TARIFF_IS_UNAVAILABLE")                                                             \
    XX(PAYMENT_TYPE_UNACCEPTABLE, "PAYMENT_TYPE_UNACCEPTABLE")                                                     \
    XX(MULTIORDER_DISALLOWED, "MULTIORDER_DISALLOWED")                                                             \
    XX(MULTIORDER_TEMP_DISALLOWED, "MULTIORDER_TEMP_DISALLOWED")                                                   \
    XX(DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_YA_PLUS, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_YA_PLUS") \
    XX(                                                                                                            \
        DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS,                                                 \
        "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS"                                                \
    )                                                                                                              \
    XX(COOP_ACCOUNT_UNAVAILABLE, "COOP_ACCOUNT_UNAVAILABLE")                                                       \
    XX(AGENT_PAYMENT_UNAVAILABLE, "AGENT_PAYMENT_UNAVAILABLE")                                                     \
    XX(PERSONAL_WALLET_INSUFFICIENT_FUNDS, "PERSONAL_WALLET_INSUFFICIENT_FUNDS")                                   \
    XX(COMPLEMENT_UNAVAILABLE, "COMPLEMENT_UNAVAILABLE")                                                           \
    XX(COMPLEMENT_PAYMENT_CHANGED, "COMPLEMENT_PAYMENT_CHANGED")                                                   \
    XX(CORP_ZONE_UNAVAILABLE, "CORP_ZONE_UNAVAILABLE")                                                             \
    XX(CORP_CANNOT_ORDER, "CORP_CANNOT_ORDER")                                                                     \
    XX(DESTINATION_ZONE_RESTRICTION_ERROR, "DESTINATION_ZONE_RESTRICTION_ERROR")                                   \
    XX(PAYMENT_TYPE_MISMATCHES_OFFER, "PAYMENT_TYPE_MISMATCHES_OFFER")                                             \
    XX(CORP_PAYMENT_METHOD_MISMATCHES_OFFER, "CORP_PAYMENT_METHOD_MISMATCHES_OFFER")                               \
    XX(DOOR_TO_DOOR_COMMENT_NOT_FILLED, "DOOR_TO_DOOR_COMMENT_NOT_FILLED")                                         \
    XX(CASH_RESTRICTED_BY_TAGS, "CASH_RESTRICTED_BY_TAGS")                                                         \
    XX(TARIFFZONE_NOT_FOUND, "TARIFFZONE_NOT_FOUND")                                                               \
    XX(TARIFF_MULTIORDER_EXCEEDED, "TARIFF_MULTIORDER_EXCEEDED")                                                   \
    XX(NO_USER_EMAIL, "NO_USER_EMAIL")                                                                             \
    XX(DELIVERY_RESTRICTED, "DELIVERY_RESTRICTED")                                                                 \
    XX(PREORDER_UNAVAILABLE, "PREORDER_UNAVAILABLE")                                                               \
    XX(MULTIORDER_DISALLOWED_FOR_SPAMMER, "MULTIORDER_DISALLOWED_FOR_SPAMMER")                                     \
    XX(MAAS_FLOW_FAILED, "MAAS_FLOW_FAILED")                                                                       \
    XX(AGENT_APPLICATION_CHANGED, "AGENT_APPLICATION_CHANGED")                                                     \
    XX(ORDER_DRAFT_EXPIRED, "ORDER_DRAFT_EXPIRED")

// integration-api-legacy/src/protocol/orderkit/models/orderdraft_check_lifetime_point.hpp
#define ORDERDRAFT_CHECK_LIFETIME_POINT_ENUM_MAP(XX) \
    XX(CommitInit, "commit_init")                    \
    XX(CommitPendingStart, "commit_pending_start")   \
    XX(CommitPendingFinish, "commit_pending_finish")

// integration-api-legacy/src/protocol/orderkit/utils.hpp
#define OPPORTUNITIES_ENUM_MAP(XX) \
    XX(Allowed, "allowed")         \
    XX(Disallowed, "disallowed")   \
    XX(NotModified, "not_modified")

// intercity-bdui-proxy/src/di/intercity_checkout_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 11

// intercity-bdui-proxy/src/di/intercity_due_selector_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 12

// intercity-bdui-proxy/src/di/intercity_order_batches_list_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 16

// internal-routestats/src/protocol/common/decoupling.hpp
#define STAGES_ENUM_MAP(XX)                    \
    XX(kCalculatingOffer, "calculating_offer") \
    XX(kOrdercommit, "ordercommit")

// internal-routestats/src/protocol/common/decoupling.hpp
#define ERROR_REASON_ENUM_MAP(XX)                     \
    XX(kGetCorpTariffFail, "get_corp_tarif_fail")     \
    XX(kPluginInternalError, "plugin_internal_error") \
    XX(kNoTariffPrice, "no_tariff_price")             \
    XX(kUnknownError, "unknown_error")

// internal-routestats/src/protocol/common/preorder/unavailable_reason.hpp
#define UNAVAILABLE_REASONS_ENUM_MAP(XX)                                            \
    XX(kPreorderUnavailableForDue, "preorder_unavailable_for_due")                  \
    XX(kPreorderUnavailableForPaymentType, "preorder_unavailable_for_payment_type") \
    XX(kPreorderUnavailableForTariff, "preorder_unavailable_for_tariff")            \
    XX(kPreorderUnavailableForRequirements, "preorder_unavailable_for_requirements")

// layers/src/utils/filtering/object_filtering_test.cpp
#define ASSERT_CLOSE_BBOXES(first, second) \
    ASSERT_TRUE(geometry::AreCloseBoundingBoxes(first, second)) << "first: " << (first) << "\nsecond: " << (second);

// market-advert-search-incuts/src/core/error_log/impl/metrics/metrics.hpp
#define MARKET_ADVERT_SEARCH_INCUTS_CORE_ERROR_LOG_IMPL_METRICS_METRICS_INL_HPP

// market-advert-search-incuts/src/core/error_log/logger_base.hpp
#define ECPM_LOG_ERROR_TO(logger, code) logger.CreateEntry(code)

// market-advert-search-incuts/src/core/externals/tests/report_base_client_test.cpp
#define EXPECT_EQ_PROTO(lhs, rhs) EXPECT_PRED2(::google::protobuf::util::MessageDifferencer::Equals, lhs, rhs);

// market-advert-search-incuts/src/core/logging/log.hpp
#define ECPM_LOG_DEBUG() LOG_DEBUG()

// market-advert-search-incuts/src/core/logging/log.hpp
#define ECPM_LOG_INFO() LOG_INFO()

// market-advert-search-incuts/src/core/logging/log.hpp
#define ECPM_LOG_WARN() LOG_WARNING()

// market-advert-search-incuts/src/core/logging/log.hpp
#define ECPM_LOG_ERROR(code) ECPM_LOG_ERROR_TO(market_advert_search_incuts::core::error_log::GetErrorLogger(), code)

// market-advertex/src/utils/bid_storage_request.cpp
#define LOG_AND_INCREMENT_WARNING() \
    ++response.warnings_count;      \
    LOG_WARNING()

// market-blender/src/common/query_storage/component.cpp
#define GET_REQUEST_WITH_METRIC(field, build_key, metric_name)                                   \
    do {                                                                                         \
        if (request.has_##field()) {                                                             \
            auto handle = [&response](std::string_view raw) {                                    \
                if (!response.mutable_##field()->ParseFromString(raw)) {                         \
                    NMarket::NLogging::LogError() << "Cannot parse " #field " from query cache"; \
                    response.clear_##field();                                                    \
                }                                                                                \
            };                                                                                   \
            keys.push_back(build_key(request.field()));                                          \
            handles.emplace_back(#metric_name, std::move(handle));                               \
        }                                                                                        \
    } while (false)

// market-blender/src/common/query_storage/component.cpp
#define GET_REQUEST(field, build_key) GET_REQUEST_WITH_METRIC(field, build_key, field)

// market-blender/src/common/query_storage/component.cpp
#define SET_REQUEST(field, build_key)                      \
    do {                                                   \
        if (auto entry = PROTO_TO_PTR(request, field)) {   \
            field = utils::Async(#field, [this, entry]() { \
                cacher_->SaveRequestAndWaitForResponse(    \
                    SERVICE_NAME,                          \
                    build_key(entry->key()),               \
                    entry->value().SerializeAsString(),    \
                    entry->ttl()                           \
                );                                         \
            });                                            \
        }                                                  \
    } while (false)

// market-blender/src/common/request/external_data.hpp
#define PROTO_TO_PTR_OR(obj, field, else_value) (obj.Has##field() ? &obj.Get##field() : else_value)

// market-blender/src/common/request/external_data.hpp
#define PROTO_TO_PTR(obj, field) PROTO_TO_PTR_OR(obj, field, nullptr)

// market-blender/src/common/request/external_data.hpp
#define PROTO_TO_MUT_PTR(obj, field) (obj.Has##field() ? obj.Mutable##field() : nullptr)

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_RANGE_TO_FILTER_REPEATED(request_field, filter_field) \
    filter.mutable_##filter_field()->Assign(request.request_field.begin(), request.request_field.end())

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_VECTOR_TO_FILTER_JOINED(request_field, filter_field)             \
    if (!request.request_field.empty()) {                                     \
        filter.set_##filter_field(JoinCommaSeparated(request.request_field)); \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_OPTIONAL_TO_FILTER(request_field, filter_field) \
    if (request.request_field) {                             \
        filter.set_##filter_field(*request.request_field);   \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_OPTIONAL_TO_FILTER_STRINGIFIED(request_field, filter_field)       \
    if (request.request_field) {                                               \
        filter.set_##filter_field(TStringBuilder() << *request.request_field); \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_OPTIONAL_BOOL_TO_FILTER_01(request_field, filter_field)   \
    if (request.request_field) {                                       \
        filter.set_##filter_field(*request.request_field ? "1" : "0"); \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_OPTIONAL_COMMA_TO_FILTER_REPEATED(request_field, filter_field)             \
    if (request.request_field) {                                                        \
        AssignCommaSeparated(*filter.mutable_##filter_field(), *request.request_field); \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_REPEATED_TO_REQUEST_VECTOR(filter_field, request_field) \
    request.request_field.assign(filter.filter_field().begin(), filter.filter_field().end())

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_JOINED_TO_REQUEST_VECTOR(filter_field, request_field) \
    AssignRepeatedString(filter.has_##filter_field(), filter.filter_field(), request.request_field)

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_TO_REQUEST_OPTIONAL(filter_field, request_field) \
    AssignOptionalString(filter.has_##filter_field(), filter.filter_field(), request.request_field)

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_TO_REQUEST_INT(filter_field, request_field)                                                 \
    if (!AssignInt(filter.has_##filter_field(), filter.filter_field(), request.request_field, #filter_field)) { \
        return;                                                                                                 \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_TO_REQUEST_BOOL(filter_field, request_field)                                                 \
    if (!AssignBool(filter.has_##filter_field(), filter.filter_field(), request.request_field, #filter_field)) { \
        return;                                                                                                  \
    }

// market-blender/src/common/request/search_refinement_state.cpp
#define COPY_FILTER_REPEATED_JOINED_TO_REQUEST_OPTIONAL(filter_field, request_field) \
    AssignJoinedString(filter.filter_field(), request.request_field)

// market-blender/src/core/debug/tracer.hpp
#define TRACE_BLENDER_DEBUG(where, what, ...) TRACE_ME(::NMarketBlender::GetBlenderTrace(where, what, __VA_ARGS__))

// market-blender/src/core/normalization/macros_utils.hpp
#define ASSIGN_IF_HAS_VALUE(field, proto_message)                                  \
    if (proto_message.has_##field()) {                                             \
        if constexpr (std::is_floating_point_v<decltype(proto_message.field())>) { \
            FillFloating(result.field, proto_message.field());                     \
        } else {                                                                   \
            result.field = proto_message.field();                                  \
        }                                                                          \
    }

// market-blender/src/core/normalization/macros_utils.hpp
#define FILL_IF_HAS_VALUE(field, proto_message, fill_func)                 \
    if (proto_message.has_##field()) {                                     \
        fill_func(proto_message.field(), EmplaceIfOptional(result.field)); \
    }

// market-blender/src/core/normalization/macros_utils.hpp
#define FILL_ARRAY_IF_NOT_EMPTY(field, proto_message, fill_func)   \
    if (int size = proto_message.field##_size()) {                 \
        auto& result_field = EmplaceIfOptional(result.field);      \
        result_field.reserve(size);                                \
        for (const auto& proto_element : proto_message.field()) {  \
            fill_func(proto_element, result_field.emplace_back()); \
        }                                                          \
    }

// market-blender/src/core/normalization/macros_utils.hpp
#define COPY_ARRAY_IF_NOT_EMPTY(field, proto_message) FILL_ARRAY_IF_NOT_EMPTY(field, proto_message, DoCopyAssign)

// market-blender/src/core/render/snippets_map.cpp
#define INSTANTIATE(Type)                                        \
    template void TSnippetsMap::CreateSnippetServiceProxyData(   \
        std::string_view,                                        \
        std::string_view,                                        \
        const Type&,                                             \
        std::optional<handlers::SnippetServiceProxyDataObject>&, \
        std::optional<handlers::SnippetServiceProxyDataString>&  \
    );

// market-blender/src/grpc/handlers/go_handle.cpp
#define NAMED_CGI_PARAM(NAME, VALUE) fmt::format("{}={}", NAME, VALUE)

// market-blender/src/grpc/handlers/go_handle.cpp
#define CGI_PARAM(X) NAMED_CGI_PARAM(#X, X)

// market-blender/src/incuts/context_tail_incut/general_context_incut.cpp
#define FILL_VALUE(result, proto_message, field)   \
    if (proto_message.has_##field()) {             \
        result.field = proto_message.Get##field(); \
    }

// market-bolat/src/custom/logger.hpp
#define LOG_BOLAT_DEBUG() LOG(market_bolat::logger::GetCtx().logging_level)

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_GET(type, param, name) param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_OPT(type, param, name) param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_TRY(type, param, name) param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_DEF(type, param, name, def) param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_MESSAGE_DEF(type, message, param, name, def) message##param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_VECTOR_DEF(type, param, name, def) param##_ = Initialize<type>(factors_map, #name);

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_GET(type, param, name)                                                                          \
        bool Has##param() const final {                                                                         \
            return param##_.has_value() || param##_.error() == InitializationError::kPresentButWrongType;       \
        }                                                                                                       \
                                                                                                                \
        type Get##param() const final {                                                                         \
            if (param##_.has_value()) {                                                                         \
                return *param##_;                                                                               \
            }                                                                                                   \
                                                                                                                \
            const std::string_view format_string =                                                              \
                param##_.error() == InitializationError::kMissing ? kMissingFactorFmt : kFactorHasWrongTypeFmt; \
            throw std::runtime_error(std::format(format_string, #name))                                         \
        }                                                                                                       \
                                                                                                                \
    private:                                                                                                    \
        std::expected<type, InitializationError> param##_;

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_TRY(type, param, name)                                                                    \
    public:                                                                                               \
        bool Has##param() const final {                                                                   \
            return param##_.has_value() || param##_.error() == InitializationError::kPresentButWrongType; \
        }                                                                                                 \
        type Get##param() const final {                                                                   \
            if (!param##_.has_value()) {                                                                  \
                if (param##_.error() == InitializationError::kMissing) {                                  \
                    return {};                                                                            \
                }                                                                                         \
                throw std::runtime_error(std::format(kFactorHasWrongTypeFmt, #name));                     \
            }                                                                                             \
            return *param##_;                                                                             \
        }                                                                                                 \
                                                                                                          \
    private:                                                                                              \
        std::expected<type, InitializationError> param##_;

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_DEF(type, param, name, def)                                              \
    public:                                                                              \
        bool Has##param() const final { return true; }                                   \
        type Get##param() const final { return param##_.has_value() ? *param##_ : def; } \
                                                                                         \
    private:                                                                             \
        std::expected<type, InitializationError> param##_;

// market-buybox/src/buybox/experiments.cpp
#define Y_VECTOR_DEF(type, param, name, def)                                             \
    public:                                                                              \
        bool Has##param() const final { return true; }                                   \
        type Get##param() const final {                                                  \
            if (param##_.has_value()) {                                                  \
                return *param##_;                                                        \
            } else {                                                                     \
                /* Может инициализировать std::vector и std::array */                    \
                if constexpr (std::is_array_v<std::remove_reference_t<decltype(def)>>) { \
                    return type(std::begin(def), std::end(def));                         \
                } else {                                                                 \
                    return type(def.begin(), def.end());                                 \
                }                                                                        \
            }                                                                            \
        }                                                                                \
                                                                                         \
    private:                                                                             \
        std::expected<type, InitializationError> param##_;

// market-buybox/src/buybox/experiments.cpp
#define Y_MESSAGE_DEF(type, message, param, name, def)                                                              \
    public:                                                                                                         \
        bool Has##message##param() const final { return true; }                                                     \
        type Get##message##param() const final { return message##param##_.has_value() ? *message##param##_ : def; } \
                                                                                                                    \
    private:                                                                                                        \
        std::expected<type, InitializationError> message##param##_;

// market-buybox/src/buybox/experiments.cpp
#define Y_PARAM_OPT(type, param, name)                                                                    \
    public:                                                                                               \
        bool Has##param() const final {                                                                   \
            return param##_.has_value() || param##_.error() == InitializationError::kPresentButWrongType; \
        }                                                                                                 \
        std::optional<type> Get##param() const final {                                                    \
            if (!param##_.has_value()) {                                                                  \
                return std::nullopt;                                                                      \
            }                                                                                             \
            return *param##_;                                                                             \
        }                                                                                                 \
                                                                                                          \
    private:                                                                                              \
        std::expected<type, InitializationError> param##_;

// market-buybox/src/buybox/filters/entity_filters.cpp
#define GET_OPTIONAL(src, field) src.has_##field() ? std::make_optional(src.field()) : std::nullopt

// market-buybox/src/common/log_helpers/declare_log_helpers.hpp
#define DECLARE_LOG_HELPERS(type)            \
    std::string ToString(const type& value); \
    ::logging::LogHelper& operator<<(::logging::LogHelper& lh, const type& value);

// market-buybox/src/common/log_helpers/declare_log_helpers.hpp
#define DEFINE_LOG_HELPERS(type)              \
    std::string ToString(const type& value) { \
        formats::json::StringBuilder builder; \
        WriteToStream(value, builder);        \
        return builder.GetString();           \
    }                                         \
    ::logging::LogHelper& operator<<(::logging::LogHelper& lh, const type& value) { return lh << ToString(value); }

// market-category-storage/src/util/logger.hpp
#define LOG_SLOWPOKE_DEBUG() LOG(market_category_storage::util::GetLoggerContext().logging_level)

// market-content-storage-userver/src/utils/card_info_parse_utils.cpp
#define LOG_CS_DEBUG() LOG_DEBUG() << kLogComponentTag << ": "

// market-content-storage-userver/src/utils/logger.hpp
#define LOG_CS_DEBUG() LOG(market_content_storage_userver::util::GetLoggerContext().logging_level)

// market-delivery-actualizer/src/utils/log/logger.hpp
#define LOG_CUSTOM_DEBUG() LOG(market_delivery_actualizer::utils::logger::GetCtx().logging_level)

// market-delivery-actualizer/src/utils/metrics.hpp
#define MDA_CREATE_METRIC(p_module, p_variable, p_tag, p_seq) I_MDA_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)

// market-delivery-actualizer/src/utils/metrics.hpp
#define MDA_CREATE_METRIC_STRUCT(p_module, p_seq) \
    I_MDA_GENERATE_BASE_METRIC_STRUCT(p_module, BOOST_PP_CAT(I_MDA_CONVERT_SEQ_X p_seq, 0))

// market-delivery-actualizer/src/utils/metrics.hpp
#define MDA_REGISTER_METRIC(p_module, p_variable, p_tag)                 \
    I_MDA_CREATE_INHERITED_METRIC_STRUCT(p_module, p_module##p_variable) \
    I_MDA_REGISTER_METRIC(p_module##p_variable, p_variable, p_tag)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)                           \
    I_MDA_GENERATE_BASE_METRIC_STRUCT(p_module, BOOST_PP_CAT(I_MDA_CONVERT_SEQ_X p_seq, 0)) \
    I_MDA_REGISTER_METRIC(p_module, p_variable, p_tag)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_CONVERT_SEQ_X(x, y) ((x, y)) I_MDA_CONVERT_SEQ_Y

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_CONVERT_SEQ_Y(x, y) ((x, y)) I_MDA_CONVERT_SEQ_X

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_CONVERT_SEQ_X0

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_CONVERT_SEQ_Y0

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_GENERATE_METRIC_SEQ(p_module, p_variable, p_tag, p_seq)                                    \
    struct p_module {                                                                                    \
        I_MDA_METRICS_DEFINE_FIELDS(p_seq)                                                               \
    };                                                                                                   \
                                                                                                         \
    [[maybe_unused]] inline void DumpMetric(::utils::statistics::Writer& writer, const p_module& stat) { \
        I_MDA_METRICS_GENERATE_DUMPING(p_seq)                                                            \
    }                                                                                                    \
                                                                                                         \
    [[maybe_unused]] inline void ResetMetric(p_module& stat) { I_MDA_METRICS_GENERATE_RESETTING(p_seq) } \
                                                                                                         \
    inline const ::utils::statistics::MetricTag<p_module> p_variable{mda::metrics::CreateMetricPath(p_tag)};

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_GENERATE_BASE_METRIC_STRUCT(p_module, p_seq)                                               \
    struct p_module {                                                                                    \
        I_MDA_METRICS_DEFINE_FIELDS(p_seq)                                                               \
    };                                                                                                   \
                                                                                                         \
    [[maybe_unused]] inline void                                                                         \
        p_module##_DumpMetricImpl(::utils::statistics::Writer& writer, const p_module& stat) {           \
        I_MDA_METRICS_GENERATE_DUMPING(p_seq)                                                            \
    }                                                                                                    \
                                                                                                         \
    [[maybe_unused]] inline void p_module##_ResetMetricImpl(p_module& stat) {                            \
        I_MDA_METRICS_GENERATE_RESETTING(p_seq)                                                          \
    }                                                                                                    \
                                                                                                         \
    [[maybe_unused]] inline void DumpMetric(::utils::statistics::Writer& writer, const p_module& stat) { \
        p_module##_DumpMetricImpl(writer, stat);                                                         \
    }                                                                                                    \
                                                                                                         \
    [[maybe_unused]] inline void ResetMetric(p_module& stat) { p_module##_ResetMetricImpl(stat); }

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_CREATE_INHERITED_METRIC_STRUCT(p_base_module, p_module)                                    \
    struct p_module : public p_base_module {};                                                           \
    [[maybe_unused]] inline void DumpMetric(::utils::statistics::Writer& writer, const p_module& stat) { \
        p_base_module##_DumpMetricImpl(writer, stat);                                                    \
    }                                                                                                    \
    [[maybe_unused]] inline void ResetMetric(p_module& stat) { p_base_module##_ResetMetricImpl(stat); }

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_REGISTER_METRIC(p_module, p_variable, p_tag) \
    inline const ::utils::statistics::MetricTag<p_module> p_variable{mda::metrics::CreateMetricPath(p_tag)};

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_REGISTER_INHERITED_STRUCT(p_base_module, p_module, p_variable, p_tag)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_DEFINE_FIELDS(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_DEFINE_FIELDS_OP, _, p_seq)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_DEFINE_FIELDS_OP(p_r, p_data, p_elem) \
    ::utils::statistics::RateCounter BOOST_PP_TUPLE_ELEM(0, p_elem);

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_GENERATE_DUMPING(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_GENERATE_DUMPING_OP, _, p_seq)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_GENERATE_DUMPING_OP(p_r, p_data, p_elem) \
    writer.ValueWithLabels(stat.BOOST_PP_TUPLE_ELEM(0, p_elem), {"type", BOOST_PP_TUPLE_ELEM(1, p_elem)});

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_GENERATE_RESETTING(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_GENERATE_RESETTING_OP, _, p_seq)

// market-delivery-actualizer/src/utils/metrics.hpp
#define I_MDA_METRICS_GENERATE_RESETTING_OP(p_r, p_data, p_seq) \
    stat.BOOST_PP_TUPLE_ELEM(0, p_seq).Store({0}, std::memory_order_seq_cst);

// market-fake-categories/src/core/resource_creator.hpp
#define DECLARE_RUN()                                                                                                 \
    template <configuration::Environment Environment>                                                                 \
    static void Run(                                                                                                  \
        const ClientType::RequestType&, ClientType::ResponseType&, resources::Context&, const handlers::Dependencies& \
    )

// market-fake-categories/src/core/resource_creator.hpp
#define DECLARE_RUN_SPECIALIZATION(Env)                                                                               \
    template <>                                                                                                       \
    static void Run<Env>(                                                                                             \
        const ClientType::RequestType&, ClientType::ResponseType&, resources::Context&, const handlers::Dependencies& \
    )

// market-fake-categories/src/lib/helpers.hpp
#define FOR_EACH_MOVE(container, iterator_name)                                  \
    for (auto iterator_name = std::make_move_iterator((container).begin()),      \
              iterator_name##__end = std::make_move_iterator((container).end()); \
         iterator_name != iterator_name##__end;                                  \
         ++iterator_name)

// market-fake-categories/src/lib/logger.hpp
#define TRACE_ME LOG(context.rearr_flags.enable_debug_log() ? logging::Level::kInfo : logging::Level::kDebug)

// market-fake-categories/src/lib/testsuite.h
#define TESTPOINT_VAR_COUNTER(variable)                             \
    TESTPOINT(#variable, ([]() {                                    \
                  auto builder = formats::json::ValueBuilder();     \
                  builder.EmplaceNocheck("value", variable.load()); \
                  return builder.ExtractValue();                    \
              })())

// market-fake-categories/src/lib/testsuite.h
#define TESTPOINT_VAR(variable, name)                     \
    TESTPOINT(name, ([]() {                               \
        auto builder = formats::json::ValueBuilder();     \
        builder.EmplaceNocheck("value", variable.load()); \
        return builder.ExtractValue();                    \
    })())

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define DYN2YT_CREATE_METRIC(p_module, p_variable, p_tag, p_seq) \
    I_DYN2YT_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq) \
    I_DYN2YT_GENERATE_METRIC_SEQ(p_module, p_variable, p_tag, BOOST_PP_CAT(I_DYN2YT_CONVERT_SEQ_X p_seq, 0))

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_CONVERT_SEQ_X(x, y) ((x, y)) I_DYN2YT_CONVERT_SEQ_Y

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_CONVERT_SEQ_Y(x, y) ((x, y)) I_DYN2YT_CONVERT_SEQ_X

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_CONVERT_SEQ_X0

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_CONVERT_SEQ_Y0

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_GENERATE_METRIC_SEQ(p_module, p_variable, p_tag, p_seq)                                    \
    struct p_module {                                                                                       \
        I_DYN2YT_METRICS_DEFINE_FIELDS(p_seq)                                                               \
    };                                                                                                      \
                                                                                                            \
    [[maybe_unused]] inline void DumpMetric(::utils::statistics::Writer& writer, const p_module& stat) {    \
        I_DYN2YT_METRICS_GENERATE_DUMPING(p_tag, p_seq)                                                     \
    }                                                                                                       \
                                                                                                            \
    [[maybe_unused]] inline void ResetMetric(p_module& stat) { I_DYN2YT_METRICS_GENERATE_RESETTING(p_seq) } \
                                                                                                            \
    inline const ::utils::statistics::MetricTag<p_module> p_variable{dyn2yt::metrics::CreateMetricPath(p_tag)};

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_DEFINE_FIELDS(p_seq) BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_DEFINE_FIELDS_OP, _, p_seq)

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_DEFINE_FIELDS_OP(p_r, p_data, p_elem) \
    ::utils::statistics::RateCounter BOOST_PP_TUPLE_ELEM(0, p_elem);

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_GENERATE_DUMPING(p_tag, p_seq) \
    BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_GENERATE_DUMPING_OP, _, p_seq)

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_GENERATE_DUMPING_OP(p_r, p_data, p_elem) \
    writer.ValueWithLabels(stat.BOOST_PP_TUPLE_ELEM(0, p_elem), {"type", BOOST_PP_TUPLE_ELEM(1, p_elem)});

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_GENERATE_RESETTING(p_seq) \
    BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_GENERATE_RESETTING_OP, _, p_seq)

// market-hide-offers-dyn2yt/src/utils/metrics.hpp
#define I_DYN2YT_METRICS_GENERATE_RESETTING_OP(p_r, p_data, p_seq) \
    stat.BOOST_PP_TUPLE_ELEM(0, p_seq).Store({0}, std::memory_order_seq_cst);

// market-link-fixer/src/global_redirects/common.hpp
#define TRY_APPLY_GLOBAL_REDIRECT(functor, context, settings)                               \
    do {                                                                                    \
        auto redirect_result = market_link_fixer::global_redirects::TryApplyGlobalRedirect< \
            decltype(functor), decltype(settings), decltype(context)                        \
        >(url, deps, context, functor, settings);                                           \
        RETURN_IF_HAS_VALUE(redirect_result);                                               \
    } while (false)

// market-link-fixer/src/lib/constants.hpp
#define PERFORMACE_PARAMS_RAW \
    "utm_source_service", "clid", "src_pof", "icookie", "baobab_event_id", "wprid", "ysclid", "vsclid", "utm_source", "utm_medium", "utm_campaign", "utm_content", "utm_term", "yclid", "ybaip"

// market-link-fixer/src/lib/helpers.hpp
#define RETURN_IF_HAS_VALUE(optional) \
    if (optional.has_value()) {       \
        return optional.value();      \
    }

// market-link-fixer/src/lib/logger.hpp
#define TRACE_ME LOG(context.is_debug_log_enabled ? logging::Level::kInfo : logging::Level::kDebug)

// market-missions/libraries/predicates/src/market_missions/predicates/models/predicates.cpp
#define EXPERIMENTS3_COMMON_INLINE __attribute__((always_inline, pure))

// market-missions/libraries/predicates/src/market_missions/predicates/models/predicates.cpp
#define EXPERIMENTS3_COMMON_NOINLINE __attribute__((noinline, cold))

// market-search-filter/src/models/index/index_resource.cpp
#define THROW_NOT_CREATED(resource)                                 \
    do {                                                            \
        if (!(resource)) {                                          \
            throw std::runtime_error("Couldn't create " #resource); \
        }                                                           \
    } while (0)

// market-snippet-service/src/custom/logger.hpp
#define LOG_SNIPPET() LOG(market_snippet_service::logger::GetLoggingLevel())

// market-unit-economy-service/src/tariff_generators/basic/validator.cpp
#define CHECK(expr)          \
    if (auto err = (expr)) { \
        return err;          \
    }

// market-walter/src/utils/nameof_utils.hpp
#define NAME_OF(name) market_walter::utils::NameOf(sizeof(typeid(name)), #name)

// market-walter/src/utils/nameof_utils.hpp
#define FORMAT_FIELD(field) \
    (field.has_value() ? std::optional<std::string>(fmt::format("{}={}", NAME_OF(field), field.value())) : std::nullopt)

// market-walter/src/utils/nameof_utils.hpp
#define FORMAT_ENUM_FIELD(field)                                                                                     \
    (                                                                                                                \
        field.has_value() ?                                                                                          \
            std::optional<std::string>(fmt::format("{}={}", NAME_OF(field), ToString(field.value()))) : std::nullopt \
    )

// mini-fts/src/grpc/helpers.cpp
#define CREATE_MEMBER_CHECKER(member)                                  \
    template <typename T>                                              \
    struct has_##member {                                              \
        template <typename U>                                          \
        static char Check(decltype(&U::member));                       \
        template <typename U>                                          \
        static int Check(...);                                         \
                                                                       \
        static const bool value = sizeof(Check<T>(0)) == sizeof(char); \
    };                                                                 \
    template <typename T>                                              \
    constexpr bool has_##member##_v = has_##member<T>::value;

// offer-layout/src/layout/common/variant_value.hpp
#define VARIANT_VALUE(setter, ...)                                       \
    ([&]() -> ::offer_layout::proto::VariantValue {                      \
        ::offer_layout::proto::VariantValue _offer_layout_variant_value; \
        _offer_layout_variant_value.setter(__VA_ARGS__);                 \
        return _offer_layout_variant_value;                              \
    }())

// offerinfo/src/custom/logger.hpp
#define LOG_OFFER_DEBUG() LOG(market_offerinfo::logger::GetCtx().logging_level)

// offers/src/bike/component_system.hpp
#define REGISTER_COMPONENT(...)                                                         \
    static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_); \
    template <>                                                                         \
    constexpr bool components::kHasValidate<__VA_ARGS__> = true;

// offers/src/bike/component_system.hpp
#define REGISTER_COMPONENT_NOSCHEMA(...)                                                \
    static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_); \
    template <>                                                                         \
    constexpr bool components::kHasValidate<__VA_ARGS__> = false;

// offers/src/bike/component_system.hpp
#define DECLARE_COMPONENT(...) static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_)(true);

// order-maker/src/protocol/config/totw_config.hpp
#define CURRENT_PRICES_WORK_MODE_ENUM_MAP(XX) \
    XX(kOldWay, "oldway")                     \
    XX(kNewWay, "newway")                     \
    XX(kDryRun, "dryrun")                     \
    XX(kTryOut, "tryout")

// order-maker/src/protocol/metrics/metric_storage.cpp
#define INIT_ORDER_429_METRIC(name, reason)                                                                     \
    name.reserve(order_too_many_handlers.size());                                                               \
    for (const auto& handler : order_too_many_handlers) {                                                       \
        name.emplace(handler, InitMetric(                                                                       \
            solomon, solomon::Metric::Labels{{"sensor", "order_429"}, {"reason", reason}, {"handler", handler}} \
        ));                                                                                                     \
    }

// order-maker/src/protocol/models/order/pin.hpp
#define MODELS_ORDER_PIN_HPP

// order-maker/src/protocol/orderkit/exceptions.hpp
#define COMMIT_ERROR_ENUM_MAP(XX)                                                                                      \
    XX(kMulticlassOrderWithoutPricingData, "MULTICLASS_ORDER_WITHOUT_PRICING_DATA")                                    \
    XX(kForcedSurgeChanged, "FORCED_SURGE_CHANGED")                                                                    \
    XX(kPriceChanged, "PRICE_CHANGED")                                                                                 \
    XX(kCantConstructRoute, "CANT_CONSTRUCT_ROUTE")                                                                    \
    XX(kRouteOverClosedBorder, "ROUTE_OVER_CLOSED_BORDER")                                                             \
    XX(kOfferNotFound, "OFFER_NOT_FOUND")                                                                              \
    XX(kOrderNotFound, "ORDER_NOT_FOUND")                                                                              \
    XX(kUnknownCard, "UNKNOWN_CARD")                                                                                   \
    XX(kBadPaymentMethod, "BAD_PAYMENT_METHOD")                                                                        \
    XX(kDebtUser, "DEBT_USER")                                                                                         \
    XX(kTooManyConcurrentOrders, "TOO_MANY_CONCURRENT_ORDERS")                                                         \
    XX(kCorpGlobalDisabled, "CORP_GLOBAL_DISABLED")                                                                    \
    XX(kNotCorpClient, "NOT_CORP_CLIENT")                                                                              \
    XX(kCorpClassDisabled, "CORP_CLASS_DISABLED")                                                                      \
    XX(kCorpLimitExceeded, "CORP_LIMIT_EXCEEDED")                                                                      \
    XX(kCorpCityDisabled, "CORP_CITY_DISABLED")                                                                        \
    XX(kCorpDeactivateThresholdError, "CORP_DEACTIVATE_THRESHOLD_ERROR")                                               \
    XX(kCorpInactiveContractError, "CORP_INACTIVE_CONTRACT_ERROR")                                                     \
    XX(kCorpServiceError, "CORP_SERVICE_ERROR")                                                                        \
    XX(kWrongRequirements, "WRONG_REQUIREMENTS")                                                                       \
    XX(kPaymentTypeCardUnsupported, "PAYMENT_TYPE_CARD_UNSUPPORTED")                                                   \
    XX(kPaymentTypeCorpUnsupported, "PAYMENT_TYPE_CORP_UNSUPPORTED")                                                   \
    XX(kPaymentTypePersonalWalletUnsupported, "PAYMENT_TYPE_PERSONAL_WALLET_UNSUPPORTED")                              \
    XX(kPaymentTypeCashUnsupported, "PAYMENT_TYPE_CASH_UNSUPPORTED")                                                   \
    XX(kPaymentTypeCoopAccountUnsupported, "PAYMENT_TYPE_COOP_ACCOUNT_UNSUPPORTED")                                    \
    XX(kPaymentTypeCargocorpUnsupported, "PAYMENT_TYPE_CARGOCORP_UNSUPPORTED")                                         \
    XX(kPaymentTypeSbpNotSupported, "PAYMENT_TYPE_SBP_NOT_SUPPORTED")                                                  \
    XX(kInvalidPhoneNumber, "INVALID_PHONE_NUMBER")                                                                    \
    XX(kPartnerOrderLimitExceeded, "PARTNER_ORDER_LIMIT_EXCEEDED")                                                     \
    XX(kFraudDetected, "FRAUD_DETECTED")                                                                               \
    XX(kNeedCardAntifraud, "NEED_CARD_ANTIFRAUD")                                                                      \
    XX(kRaceCondition, "RACE_CONDITION")                                                                               \
    XX(kTariffIsRestricted, "TARIFF_IS_RESTRICTED")                                                                    \
    XX(kTariffIsUnavailable, "TARIFF_IS_UNAVAILABLE")                                                                  \
    XX(kPaymentTypeUnacceptable, "PAYMENT_TYPE_UNACCEPTABLE")                                                          \
    XX(kMultiorderDisallowed, "MULTIORDER_DISALLOWED")                                                                 \
    XX(kMultiorderTempDisallowed, "MULTIORDER_TEMP_DISALLOWED")                                                        \
    XX(kDisabledPaymentTypePersonalWalletIfNoYaPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_YA_PLUS")            \
    XX(kDisabledPaymentTypePersonalWalletIfNoCashbackPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS") \
    XX(kCoopAccountUnavailable, "COOP_ACCOUNT_UNAVAILABLE")                                                            \
    XX(kAgentPaymentUnavailable, "AGENT_PAYMENT_UNAVAILABLE")                                                          \
    XX(kPersonalWalletInsufficientFunds, "PERSONAL_WALLET_INSUFFICIENT_FUNDS")                                         \
    XX(kComplementUnavailable, "COMPLEMENT_UNAVAILABLE")                                                               \
    XX(kComplementPaymentChanged, "COMPLEMENT_PAYMENT_CHANGED")                                                        \
    XX(kCorpZoneUnavailable, "CORP_ZONE_UNAVAILABLE")                                                                  \
    XX(kCorpCannotOrder, "CORP_CANNOT_ORDER")                                                                          \
    XX(kDestinationZoneRestrictionError, "DESTINATION_ZONE_RESTRICTION_ERROR")                                         \
    XX(kOrderWithoutDestinationNotAllowedForPaymentMethod, "ORDER_WITHOUT_DESTINATION_NOT_ALLOWED_FOR_PAYMENT_METHOD") \
    XX(kPaymentTypeMismatchesOffer, "PAYMENT_TYPE_MISMATCHES_OFFER")                                                   \
    XX(kCorpPaymentMethodMismatchesOffer, "CORP_PAYMENT_METHOD_MISMATCHES_OFFER")                                      \
    XX(kDoorToDoorCommentNotFilled, "DOOR_TO_DOOR_COMMENT_NOT_FILLED")                                                 \
    XX(kCashRestrictedByTags, "CASH_RESTRICTED_BY_TAGS")                                                               \
    XX(kTariffzoneNotFound, "TARIFFZONE_NOT_FOUND")                                                                    \
    XX(kTariffMultiorderExceeded, "TARIFF_MULTIORDER_EXCEEDED")                                                        \
    XX(kNoSmilesPaymentAuthorizationData, "NO_SMILES_PAYMENT_AUTHORIZATION_DATA")                                      \
    XX(kWrongExternalSuperappName, "WRONG_EXTERNAL_SUPERAPP_NAME")                                                     \
    XX(kExternalSuperappCommitFailed, "EXTERNAL_SUPERAPP_COMMIT_FAILED")                                               \
    XX(kNoUserEmail, "NO_USER_EMAIL")                                                                                  \
    XX(kDeliveryRestricted, "DELIVERY_RESTRICTED")                                                                     \
    XX(kPreorderUnavailable, "PREORDER_UNAVAILABLE")                                                                   \
    XX(kMultiorderDisallowedForSpammer, "MULTIORDER_DISALLOWED_FOR_SPAMMER")                                           \
    XX(kMaasFlowFailed, "MAAS_FLOW_FAILED")                                                                            \
    XX(kAgentApplicationChanged, "AGENT_APPLICATION_CHANGED")                                                          \
    XX(kOrderDraftExpired, "ORDER_DRAFT_EXPIRED")

// order-maker/src/protocol/orderkit/models/order_service_type.hpp
#define ORDER_SERVICE_TYPE_ENUM_MAP(XX) \
    XX(kCargo, "cargo")                 \
    XX(kTaxi, "taxi")

// order-maker/src/protocol/orderkit/models/orderdraft_check_lifetime_point.hpp
#define ORDERDRAFT_CHECK_LIFETIME_POINT_ENUM_MAP(XX) \
    XX(kCommitInit, "commit_init")                   \
    XX(kCommitPendingStart, "commit_pending_start")  \
    XX(kCommitPendingFinish, "commit_pending_finish")

// order-maker/src/protocol/orderkit/utils.hpp
#define OPPORTUNITIES_ENUM_MAP(XX) \
    XX(kAllowed, "allowed")        \
    XX(kDisallowed, "disallowed")  \
    XX(kNotModified, "not_modified")

// order-taxi-messenger/src/protocol/views/extra_phone.hpp
#define EXTRA_PHONE_TYPE_ENUM_MAP(XX) \
    XX(kMain, "main")                 \
    XX(kExtra, "extra")

// partner-orders-core-api/src/protocol/orderkit/exceptions.hpp
#define COMMIT_ERROR_ENUM_MAP(XX)                              \
    XX(kTooManyConcurrentOrders, "TOO_MANY_CONCURRENT_ORDERS") \
    XX(kOrderDraftExpired, "ORDER_DRAFT_EXPIRED")

// personal/src/models/common_types.cpp
#define DATA_TYPE_PARSER(FIELD_NAME, TYPE)                                                                        \
    if (value.FIELD_NAME) {                                                                                       \
        if (IsMultiTypeParsing(result)) {                                                                         \
            return JsonMultiTypeError();                                                                          \
        }                                                                                                         \
        const auto data_type_value =                                                                              \
            NormalizeDataTypeToJson(handlers::DataTypeExtended::TYPE##s, *value.FIELD_NAME, normalization_prefs); \
        result.type = handlers::CommonTypeEnum::TYPE;                                                             \
        result.value = data_type_value.value;                                                                     \
        if (data_type_value.normalized) {                                                                         \
            result.normalized.emplace().FIELD_NAME = data_type_value.normalized;                                  \
        }                                                                                                         \
        result.error = data_type_value.error;                                                                     \
        if (!result.error.empty()) {                                                                              \
            return result;                                                                                        \
        }                                                                                                         \
    }

// persuggest/src/utils/log.hpp
#define XLOG_DEBUG() LOG_DEBUG() << __FILE__ << ":" << __LINE__ << "[" << __func__ << "]" << " xlog: "

// pin-storage/src/storages/pin_id_encoder.cpp
#define XXH_INLINE_ALL

// price-estimate-api/src/utils/metrics/common.hpp
#define DEFINE_TAG(ClassName, TagName) \
    template <>                        \
    const ClassName::ValueType ClassName::MetricTagHolder::kMetricTag(#TagName);

// pro-datum/src/grpc/v1/service_impl.hpp
#define DECLARE_METHOD(name)                                                        \
    private:                                                                        \
        ResponseExtractor<name##Result>::Type name(name##Request&& request, handlers::Dependencies&& dependencies) const; \
                                                                                    \
    public:                                                                         \
        name##Result name(CallContext& context, name##Request&& request) override { \
            using ThisType = typename std::remove_reference_t<decltype(*this)>;     \
            return WrapMethod<                                                      \
                name##Request,                                                      \
                ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request)); \
        }

// qc-gateway/src/yandex/business/quality-control/v1/service_impl.hpp
#define DECLARE_METHOD(name)                                                        \
    private:                                                                        \
        ResponseExtractor<name##Result>::Type name(                                 \
            name##Request&& request,                                                \
            handlers::Dependencies&& dependencies,                                  \
            const aip_161_field_mask::FieldMask& field_mask,                        \
            const qc_gate_common::permissions::Permissions& permissions,            \
            ugrpc::server::CallContext& context                                     \
        ) const;                                                                    \
                                                                                    \
    public:                                                                         \
        name##Result name(CallContext& context, name##Request&& request) override { \
            using ThisType = typename std::remove_reference_t<decltype(*this)>;     \
            return WrapMethod<                                                      \
                name##Request,                                                      \
                ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request)); \
        }

// quality-control/qc-core/src/utils/class.hpp
#define DELETE_COPY(name)                  \
    name(name&&) = default;                \
    name(const name&) = delete;            \
                                           \
    name& operator=(name&&) = default;     \
    name& operator=(const name&) = delete; \
                                           \
    virtual ~name() = default;

// quality-control/qc-core/src/utils/class.hpp
#define DELETE_COPY_MOVE(name)             \
    name(name&&) = delete;                 \
    name(const name&) = delete;            \
                                           \
    name& operator=(name&&) = delete;      \
    name& operator=(const name&) = delete; \
                                           \
    virtual ~name() = default;

// quality-control/qc-core/src/yandex/business/quality-control/qc-core/v1/service_impl.hpp
#define DECLARE_METHOD(name)                                                        \
    private:                                                                        \
        ResponseExtractor<name##Result>::Type name(                                 \
            name##Request&& request,                                                \
            handlers::Dependencies&& dependencies,                                  \
            const aip_161_field_mask::FieldMask& field_mask                         \
        ) const;                                                                    \
                                                                                    \
    public:                                                                         \
        name##Result name(CallContext& context, name##Request&& request) override { \
            using ThisType = typename std::remove_reference_t<decltype(*this)>;     \
            return WrapMethod<                                                      \
                name##Request,                                                      \
                ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request)); \
        }

// quality-control/qc-entities/src/yandex/business/quality-control/qc-entities/v1/service_impl.hpp
#define DECLARE_BIDIRECTIONAL_STREAM(name)                                                                  \
    private:                                                                                                \
        ResponseExtractor<name##Result>::Type name(name##Request&& request, handlers::Dependencies&& dependencies) const; \
                                                                                                            \
    public:                                                                                                 \
        name##Result name(CallContext& context, name##ReaderWriter& stream) override {                      \
            using ThisType = typename std::remove_reference_t<decltype(*this)>;                             \
            return WrapBidiretionalStream<name##Request, name##Response>(&ThisType::name, context, stream); \
        }

// quality-control/qc-entities/src/yandex/business/quality-control/qc-entities/v1/service_impl.hpp
#define DECLARE_SERVER_STREAM(name)                                                                            \
    private:                                                                                                   \
        void name(name##Request&& request, handlers::Dependencies&& dependencies, name##Writer& stream) const; \
                                                                                                               \
    public:                                                                                                    \
        ugrpc::server::StreamingResult<name##Response>                                                         \
        name(ugrpc::server::CallContext& context, name##Request&& request, name##Writer& stream) override {    \
            using ThisType = typename std::remove_reference_t<decltype(*this)>;                                \
            return WrapServerStream<                                                                           \
                name##Request,                                                                                 \
                name##Response,                                                                                \
                ThisType>(&ThisType::name, std::move(request), context, stream);                               \
        }

// rtxaron-events/src/utils/utils.hpp
#define SET_PROTO_FIELD(ProtoPtr, Field, OptValue) \
    if (OptValue.has_value()) {                    \
        ProtoPtr->set_##Field(OptValue.value());   \
    }
// scooters-coords/src/metrics/common.hpp
#define SCOOTERS_COORDS_DEFINE_STD_HASH_FOR_LABELS(Type) \
    template <>                                          \
    struct std::hash<Type> : metrics::LabelsHash<Type> {}

// scooters-core/src/order/checks/geo/checker.cpp
#define L(x) #x "=" << (x)

// scooters-ops/src/pro/builders/macros.hpp
#define SCOOTERS_OPS_BUILDER_SINGLE_PARAM_METHOD(MethodName, ParamType, ParamName, ...) \
    auto& MethodName(ParamType ParamName)& {                                            \
        __VA_ARGS__                                                                     \
        return AsSelf();                                                                \
    };                                                                                  \
    auto&& MethodName(ParamType ParamName)&& {                                          \
        __VA_ARGS__                                                                     \
        return std::move(AsSelf());                                                     \
    };                                                                                  \
    static_assert(true, "SCOOTERS_OPS_BUILDER_SINGLE_PARAM_METHOD requires semicolon")

// scooters-ops/src/pro/builders/macros.hpp
#define SCOOTERS_OPS_BUILDER_NO_PARAM_METHOD(MethodName, ...) \
    auto& MethodName()& {                                     \
        __VA_ARGS__                                           \
        return AsSelf();                                      \
    };                                                        \
    auto&& MethodName()&& {                                   \
        __VA_ARGS__                                           \
        return std::move(AsSelf());                           \
    };                                                        \
    static_assert(true, "SCOOTERS_OPS_BUILDER_NO_PARAM_METHOD requires semicolon")

// scooters-pro-rent/src/utils/concepts_and_traits.hpp
#define DECLARE_SCOOTERS_OPS_TRAITS_FUNCTION_PTR_BASE                                \
    using is_callable_traits = std::true_type;                                       \
    using Result = ResultType;                                                       \
    using ArgsTuple = std::tuple<Args...>;                                           \
    template <typename ResultTypeToCheck>                                            \
    constexpr static bool IsCorrectResult = std::same_as<ResultTypeToCheck, Result>; \
    template <typename... ArgsToCheck>                                               \
    constexpr static bool IsCorrectArgs = std::same_as<std::tuple<ArgsToCheck...>, ArgsTuple>;

// scooters-pro-rent/src/utils/concepts_and_traits.hpp
#define DECLARE_SCOOTERS_OPS_TRAITS_MEMBER_BASE     \
    DECLARE_SCOOTERS_OPS_TRAITS_FUNCTION_PTR_BASE   \
    using Object = std::remove_const_t<ObjectType>; \
    template <utils::Object T>                      \
    constexpr static bool IsObject = std::same_as<std::remove_cvref_t<T>, Object>;

// segments-provider/src/models/validation/proposed_query_changes.cpp
#define TO_CHANGED_FIELD_OVERRIDE(new_val, old_val, is_disabling, string_name, field) \
    old_val ? ToChangedField(                                                         \
        string_name,                                                                  \
        is_disabling,                                                                 \
        new_val,                                                                      \
        *old_val,                                                                     \
        &std::decay_t<decltype(new_val)>::field,                                      \
        &std::decay_t<decltype(*(old_val))>::field                                    \
    ) : ChangedField<decltype(new_val.field)>(string_name, new_val.field, kRequireChecking, kWasNotChanged)

// segments-provider/src/models/validation/proposed_query_changes.cpp
#define TO_CHANGED_FIELD(new_val, old_val, is_disabling, field) \
    TO_CHANGED_FIELD_OVERRIDE(new_val, old_val, is_disabling, FieldName(#field), field)

// segments-provider/src/models/validation/proposed_query_changes.cpp
#define TO_CHANGED_FIELD_NESTED_OVERRIDE(new_val, old_val, is_disabling, string_name, inner_field, field) \
    old_val ? ToChangedField(                                                                             \
        string_name,                                                                                      \
        is_disabling,                                                                                     \
        new_val.inner_field,                                                                              \
        old_val->inner_field,                                                                             \
        &decltype(new_val.inner_field)::field,                                                            \
        &decltype(old_val->inner_field)::field                                                            \
    ) : ChangedField<decltype(new_val.inner_field.field)>(                                                \
        string_name, new_val.inner_field.field, kRequireChecking, kWasNotChanged                          \
    )

// segments-provider/src/models/validation/proposed_query_changes.cpp
#define TO_CHANGED_FIELD_NESTED(new_val, old_val, is_disabling, inner_field, field)           \
    TO_CHANGED_FIELD_NESTED_OVERRIDE(                                                         \
        new_val, old_val, is_disabling, FieldName(#inner_field "."#field), inner_field, field \
    )

// shops/src/custom/logger.hpp
#define LOG_CUSTOM_DEBUG() LOG(market_shops::logger::GetCtx().logging_level)

// shortcuts/src/components/experiment_based_parameters.cpp
#define PARSE_FIELD(Struct, Name) value[#Name].As<decltype(Struct::Name)>()

// shuttle-control/src/utils/variant_match.hpp
#define THERE_ARE_MULTIPLE_VARIANT_MATCH_BY_DIFFERENT_PATHS

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KALKANCRYPT_H

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_DECL

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_PKCS12 0x00000001

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_KZIDCARD 0x00000002

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_KAZTOKEN 0x00000004

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_ETOKEN72K 0x00000008

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_JACARTA 0x00000010

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_X509CERT 0x00000020

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_AKEY 0x00000040

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCST_ETOKEN5110 0x00000080

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_DER 0x00000101

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_PEM 0x00000102

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_B64 0x00000104

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_CA 0x00000201

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_INTERMEDIATE 0x00000202

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERT_USER 0x00000204

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_USE_NOTHING 0x00000401

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_USE_CRL 0x00000402

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_USE_OCSP 0x00000404

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_INCL_C14N 0x01000001

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_INCL_C14NCOMMENT 0x01000002

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_INCL_C14N11 0x01000004

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_INCL_C14N11COMMENT 0x01000008

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_EXCL_C14N 0x01000010

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XML_EXCL_C14NCOMMENT 0x01000020

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_INCL_C14N 0x01000040

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_INCL_C14NCOMMENT 0x01000080

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_INCL_C14N11 0x01000100

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_INCL_C14N11COMMENT 0x01000200

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_EXCL_C14N 0x01000400

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_XMLC_EXCL_C14NCOMMENT 0x01000800

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_COUNTRYNAME 0x00000801

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_SOPN 0x00000802

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_LOCALITYNAME 0x00000803

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_ORG_NAME 0x00000804

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_ORGUNIT_NAME 0x00000805

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_COMMONNAME 0x00000806

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_COUNTRYNAME 0x00000807

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_SOPN 0x00000808

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_LOCALITYNAME 0x00000809

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_COMMONNAME 0x0000080a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_GIVENNAME 0x0000080b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_SURNAME 0x0000080c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_SERIALNUMBER 0x0000080d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_EMAIL 0x0000080e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_ORG_NAME 0x0000080f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_ORGUNIT_NAME 0x00000810

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_BC 0x00000811

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_DC 0x00000812

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_NOTBEFORE 0x00000813

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_NOTAFTER 0x00000814

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_KEY_USAGE 0x00000815

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_EXT_KEY_USAGE 0x00000816

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_AUTH_KEY_ID 0x00000817

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJ_KEY_ID 0x00000818

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_CERT_SN 0x00000819

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_ISSUER_DN 0x0000081a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SUBJECT_DN 0x0000081b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_SIGNATURE_ALG 0x0000081c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_PUBKEY 0x0000081d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_CERTPROP_POLICIES_ID 0x0000081e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_SIGN_DRAFT 0x00000001

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_SIGN_CMS 0x00000002

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_IN_PEM 0x00000004

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_IN_DER 0x00000008

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_IN_BASE64 0x00000010

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_IN2_BASE64 0x00000020

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_DETACHED_DATA 0x00000040

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_WITH_CERT 0x00000080

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_WITH_TIMESTAMP 0x00000100

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_OUT_PEM 0x00000200

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_OUT_DER 0x00000400

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_OUT_BASE64 0x00000800

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_PROXY_OFF 0x00001000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_PROXY_ON 0x00002000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_PROXY_AUTH 0x00004000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_IN_FILE 0x00008000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_NOCHECKCERTTIME 0x00010000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_HASH_SHA256 0x00020000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_HASH_GOST95 0x00040000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KC_GET_OCSP_RESPONSE 0x00080000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_BASE 0x08F00000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OK 0x00000000

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INIT_ERROR KCR_BASE + 0x00000001

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ERROR_READ_PKCS12 KCR_BASE + 0x00000002

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ERROR_OPEN_PKCS12 KCR_BASE + 0x00000003

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INVALID_PROPID KCR_BASE + 0x00000004

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_BUFFER_TOO_SMALL KCR_BASE + 0x00000005

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERT_PARSE_ERROR KCR_BASE + 0x00000006

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INVALID_FLAG KCR_BASE + 0x00000007

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OPENFILEERR KCR_BASE + 0x00000008

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INVALIDPASSWORD KCR_BASE + 0x00000009

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERTWRONGDATE KCR_BASE + 0x0000000a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERTEXPIRED KCR_BASE + 0x0000000b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ISNOTCACERT KCR_BASE + 0x0000000c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_MEMORY_ERROR KCR_BASE + 0x0000000d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CHECKCHAINERROR KCR_BASE + 0x0000000e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CACERTKEYUSAGEERROR KCR_BASE + 0x0000000f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_VALIDTYPEERROR KCR_BASE + 0x00000010

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_BADCRLFORMAT KCR_BASE + 0x00000011

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_LOADCRLERROR KCR_BASE + 0x00000012

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_LOADCRLSERROR KCR_BASE + 0x00000013

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_UNKNOWN_ALG KCR_BASE + 0x00000015

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_KEYNOTFOUND KCR_BASE + 0x00000016

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SIGN_INIT_ERROR KCR_BASE + 0x00000017

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SIGN_ERROR KCR_BASE + 0x00000018

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ENCODE_ERROR KCR_BASE + 0x00000019

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INVALID_FLAGS KCR_BASE + 0x0000001a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERTNOTFOUND KCR_BASE + 0x0000001b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_VERIFYSIGNERROR KCR_BASE + 0x0000001c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_BASE64_DECODE_ERROR KCR_BASE + 0x0000001d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_UNKNOWN_CMS_FORMAT KCR_BASE + 0x0000001e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_GETHASHERROR KCR_BASE + 0x0000001f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CA_CERT_NOT_FOUND KCR_BASE + 0x00000020

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLSECINIT_ERROR KCR_BASE + 0x00000021

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_LOADTRUSTEDCERTSERR KCR_BASE + 0x00000022

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SIGN_INVALID KCR_BASE + 0x00000023

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_NOSIGNFOUND KCR_BASE + 0x00000024

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_DECODE_ERROR KCR_BASE + 0x00000025

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLPARSEERROR KCR_BASE + 0x00000026

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLADDIDERROR KCR_BASE + 0x00000027

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLINTERNALERROR KCR_BASE + 0x00000028

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLSETSIGNERROR KCR_BASE + 0x00000029

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OPENSSLERROR KCR_BASE + 0x0000002a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ENGINE_INITERR KCR_BASE + 0x0000002b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_NOTOKENFOUND KCR_BASE + 0x0000002c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OCSP_ADDCERTERR KCR_BASE + 0x0000002d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OCSP_PARSEURLERR KCR_BASE + 0x0000002e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OCSP_ADDHOSTERR KCR_BASE + 0x0000002f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OCSP_REQERR KCR_BASE + 0x00000030

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OCSP_CONNECTIONERR KCR_BASE + 0x00000031

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_VERIFY_NODATA KCR_BASE + 0x00000032

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_IDATTR_NOTFOUND KCR_BASE + 0x00000033

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_IDRANGE KCR_BASE + 0x00000034

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLKEYDUPERROR KCR_BASE + 0x00000035

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_XMLKEYCREATEERROR KCR_BASE + 0x00000036

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_READERNOTFOUND KCR_BASE + 0x00000037

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_GETCERTPROPERR KCR_BASE + 0x00000038

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SIGNFORMMAT KCR_BASE + 0x00000039

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INDATAFORMAT KCR_BASE + 0x0000003a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_OUTDATAFORMAT KCR_BASE + 0x0000003b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_VERIFY_INIT_ERROR KCR_BASE + 0x0000003c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_VERIFY_ERROR KCR_BASE + 0x0000003d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_HASH_ERROR KCR_BASE + 0x0000003e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SIGNHASH_ERROR KCR_BASE + 0x0000003f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CACERTNOTFOUND KCR_BASE + 0x00000040

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERTTIMEINVALID KCR_BASE + 0x00000042

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CONVERTERROR KCR_BASE + 0x00000043

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_TSACREATEQUERY KCR_BASE + 0x00000044

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CREATEOBJ KCR_BASE + 0x00000045

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CREATENONCE KCR_BASE + 0x00000046

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_HTTPERROR KCR_BASE + 0x00000047

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CADESBES_FAILED KCR_BASE + 0x00000048

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CADEST_FAILED KCR_BASE + 0x00000049

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_NOTSATOKEN KCR_BASE + 0x0000004a

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_INVALID_DIGEST_LEN KCR_BASE + 0x0000004b

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_GENRANDERROR KCR_BASE + 0x0000004c

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_SOAPNSERROR KCR_BASE + 0x0000004d

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_GETPUBKEY KCR_BASE + 0x0000004e

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_GETCERTINFO KCR_BASE + 0x0000004f

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_FILEREADERROR KCR_BASE + 0x00000050

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CHECKERROR KCR_BASE + 0x00000051

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ZIPEXTRACTERR KCR_BASE + 0x00000052

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_NOMANIFESTFILE KCR_BASE + 0x00000053

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_LIBRARYNOTINITIALIZED KCR_BASE + 0x00000101

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_ENGINELOADERR KCR_BASE + 0x00000200

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_PARAM_ERROR KCR_BASE + 0x00000300

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERT_STATUS_OK KCR_BASE + 0x00000400

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERT_STATUS_REVOKED KCR_BASE + 0x00000401

// smart-bridge-signer/kalkan/KalkanCrypt.h
#define KCR_CERT_STATUS_UNKNOWN KCR_BASE + 0x00000402

// stq-agent/src/models/stq_shard.cpp
#define ETA_UPDATE_WITH_DUP(val)                                                                  \
    ::stq_agent::common::names::mongo::stq::kEta, (val), ::stq_agent::common::names::mongo::stq:: \
        kInfoWithEta, (GetEtaWithInfo(val))

// subvention-schedule/src/helpers/exceptions.cpp
#define RETHROW_IF_NEEDED(do_rethrow) \
    if (do_rethrow) throw

// subvention-view/src/utils/log_helper.hpp
#define LOG_ERROR_IF_FAILED(condition, message)                                                    \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            const auto err_str = ::fmt::format("Validation ({}) failed: {}", #condition, message); \
            LOG_ERROR() << err_str;                                                                \
        }                                                                                          \
    } while (0)

// superapp-bdui-proxy/src/di/default_injector.hpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 17

// supportai-core/src/core/tests/assert.hpp
#define ASSERT_PROTO(lhs, rhs) ASSERT_TRUE(supportai_core::core::tests::Compare(lhs, rhs))

// supportai-core/src/core/tests/assert.hpp
#define ASSERT_PROTO_WITH_IGNORE(lhs, rhs, vector) ASSERT_TRUE(supportai_core::core::tests::Compare(lhs, rhs, vector))

// surge-calculator/src/resources/calculations_history/instance.cpp
#define PARSE_JSON_FIELD(field_name, json) field_name = json[#field_name].As<decltype(result.field_name)>()

// tags/src/utils/yt_logger.cpp
#define LOG_TAG(TagEventT) \
    LogTag<TagEventT>(yt_logger, provider_name, entity_name, entity_type, tag_name, now_timestring, ttl, active)

// tariffs-promotions/src/utils/smart_tariff_suggestion/experiment_adapters.hpp
#define CREATE_SMART_TARIFF_ADAPTER(ExperimentName, Namespace)                                                  \
    template <>                                                                                                 \
    struct ExperimentAdapter<experiments3::ExperimentName> {                                                    \
        using DisplayInfo = experiments3::Namespace::DisplayInfo;                                               \
        using Setting = experiments3::Namespace::Setting;                                                       \
        using Condition = experiments3::Namespace::Condition;                                                   \
        using AttributedContentTemplateImageItem = experiments3::Namespace::AttributedContentTemplateImageItem; \
        using AttributedContentImageItem = experiments3::Namespace::AttributedContentImageItem;                 \
        using AttributedContentTextItem = experiments3::Namespace::AttributedContentTextItem;                   \
        using SurgeIsMoreThanCondition = experiments3::Namespace::SurgeIsMoreThanCondition;                     \
        using EtaInSecondsIsLessThanCondition = experiments3::Namespace::EtaInSecondsIsLessThanCondition;       \
        using CheaperCondition = experiments3::Namespace::CheaperCondition;                                     \
        using ExpensiveCondition = experiments3::Namespace::ExpensiveCondition;                                 \
        using UltimaCondition = experiments3::Namespace::UltimaCondition;                                       \
        using DiffPricePercentCondition = experiments3::Namespace::DiffPricePercentCondition;                   \
        using DiffEtaCondition = experiments3::Namespace::DiffEtaCondition;                                     \
        using AlwaysTrueCondition = experiments3::Namespace::AlwaysTrueCondition;                               \
        using DeeplinkArrowButton = experiments3::Namespace::DeeplinkArrowButton;                               \
        using TemplateDeeplinkArrowButton = experiments3::Namespace::TemplateDeeplinkArrowButton;               \
                                                                                                                \
        static const std::string GetMetaType() { return experiments3::ExperimentName::kName; }                  \
    };

// totw/src/protocol/orderkit/exceptions.hpp
#define COMMIT_ERROR_ENUM_MAP(XX) XX(ORDER_NOT_FOUND, "ORDER_NOT_FOUND")

// totw/src/protocol/orderkit/models/order_service_type.hpp
#define ORDER_SERVICE_TYPE_ENUM_MAP(XX) \
    XX(Cargo, "cargo")                  \
    XX(Taxi, "taxi")

// ultima-mode/src/utils/json/postgres.hpp
#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    namespace traits {                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = ultima_mode::utils::json::JsonPgConverter<codegen_type>;                \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = ultima_mode::utils::json::JsonPgConverter<codegen_type>;                \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
                                                                                                  \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
                                                                                                  \
    }

// umlaas-eats/src/caches/places_catalog_storage.cpp
#define MUST_HAVE_VALUE(field)                                                \
    do {                                                                      \
        if (!field.has_value()) {                                             \
            LOG_ERROR() << "place " << place.id << " missing field: " #field; \
            return false;                                                     \
        }                                                                     \
    } while (0)

// umlaas-eats-catalog/src/utils/catalog/impl/compare_test.cpp
#define BETTER(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kBetter) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Better";

// umlaas-eats-catalog/src/utils/catalog/impl/compare_test.cpp
#define WORST(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kWorst) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Worst";

// umlaas-eats-catalog/src/utils/catalog/impl/compare_test.cpp
#define EQUAL(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kEqual) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Equal";

// united-dispatch/src/dispatch/proposition-builders/eats/common/dead_batch_solver.cpp
#define BOOST_DISABLE_PRAGMA_MESSAGE  // Отключает предупреждение из matching-algorithms.hpp

// userver-sample/src/handlers/redis_test_commands.cpp
#define EXPECT_EQ(...)                       \
    do {                                     \
        try {                                \
            ExpectEq(__VA_ARGS__);           \
        } catch (const std::exception& ex) { \
            LOG_ERROR() << ex.what();        \
            throw;                           \
        }                                    \
    } while (0)

// userver-sample/src/handlers/redis_test_commands.cpp
#define EXPECT_NOTHROW(x)                    \
    do {                                     \
        try {                                \
            x;                               \
        } catch (const std::exception& ex) { \
            LOG_ERROR() << ex.what();        \
            throw;                           \
        }                                    \
    } while (0)

// userver-sample/src/handlers/redis_test_commands.cpp
#define EXPECT_THROW(x, ex_t)                                                                                       \
    do {                                                                                                            \
        bool catched = false;                                                                                       \
        try {                                                                                                       \
            x;                                                                                                      \
        } catch (const ex_t&) {                                                                                     \
            catched = true;                                                                                         \
        }                                                                                                           \
        if (!catched) {                                                                                             \
            auto msg = std::string("exception of type '") + #ex_t + "' was not thrown in expression '" + #x + '\''; \
            LOG_ERROR() << msg;                                                                                     \
            throw std::runtime_error(msg);                                                                          \
        }                                                                                                           \
    } while (0)

// vehicle-permits/src/utils/should_cancel.hpp
#define SHOULD_CANCEL_POINT                   \
    if (engine::current_task::ShouldCancel()) \
    throw utils::should_cancel::ShouldCancelException()  // ";" has to be in code

// vehicle-permits/src/utils/should_cancel.hpp
#define SLEEP_WITH_SHOULD_CANCEL_POINT(interval) \
    engine::InterruptibleSleepFor(interval);     \
    SHOULD_CANCEL_POINT

// vehicle-permits/src/utils/unordered_requirements.hpp
#define UNORDERED_REQUIREMENTS_EQUAL_FIELDS(z, data, field) &&a.field == b.field

// vehicle-permits/src/utils/unordered_requirements.hpp
#define UNORDERED_REQUIREMENTS_OPERATOR_EQUAL(Type, fields)                                \
    inline bool operator==(const Type& a, const Type& b) noexcept {                        \
        return true BOOST_PP_SEQ_FOR_EACH(UNORDERED_REQUIREMENTS_EQUAL_FIELDS, ~, fields); \
    }

// vehicle-permits/src/utils/unordered_requirements.hpp
#define UNORDERED_REQUIREMENTS_COMBINE_FIELD(z, data, field) boost::hash_combine(seed, value.field);

// vehicle-permits/src/utils/unordered_requirements.hpp
#define UNORDERED_REQUIREMENTS_STD_HASH(Type, fields)                              \
    namespace std {                                                                \
    template <>                                                                    \
    struct hash<Type> {                                                            \
        size_t operator()(const Type& value) const noexcept {                      \
            size_t seed = 0;                                                       \
            BOOST_PP_SEQ_FOR_EACH(UNORDERED_REQUIREMENTS_COMBINE_FIELD, ~, fields) \
            return seed;                                                           \
        }                                                                          \
    };                                                                             \
    }

// vehicle-permits/src/utils/unordered_requirements.hpp
#define UNORDERED_REQUIREMENTS(nspace, Type, fields)    \
    namespace nspace {                                  \
    UNORDERED_REQUIREMENTS_OPERATOR_EQUAL(Type, fields) \
    }                                                   \
    UNORDERED_REQUIREMENTS_STD_HASH(nspace::Type, fields)

// yango-ne-communications/src/utils/span.hpp
#define DECL_SPAN_KEY(T, KEY)               \
    template <>                             \
    struct SpanKeyInfo<T> {                 \
        static constexpr auto kKey = (KEY); \
    };

// yango-wallet/src/partners/yabx/api/errors.hpp
#define DECL_YABX_API_ERROR(cls, name)                                                                \
    struct cls {                                                                                      \
        static constexpr auto kName = #name;                                                          \
        int32_t code = {};                                                                            \
        std::string message = {};                                                                     \
        constexpr std::string FormatMessage() const {                                                 \
            return fmt::format("Api responded with {}, code: {}, message: {}", kName, code, message); \
        }                                                                                             \
    };

// yango-wallet/src/utils/expect.hpp
#define YW_EXPECT(_condition, _message)                                                          \
    do {                                                                                         \
        if (_condition) {}                                                                       \
        else {                                                                                   \
            throw ::yango_wallet::utils::ExpectError(_message, std::source_location::current()); \
        }                                                                                        \
    } while (false)

// yango-wallet/src/utils/json.cpp
#define LOG_FAILURE(loc_, fmt_, ...)          \
    do {                                      \
        LOG_ERROR() << fmt::format(           \
            "{}:{}:{} in function {}: " fmt_, \
            loc_.file_name(),                 \
            loc_.line(),                      \
            loc_.column(),                    \
            loc_.function_name(),             \
            ##__VA_ARGS__                     \
        );                                    \
    } while (0)

// tests/format/src/format_ifdef_input.cpp
#define FORMAT_IFDEF_FIXTURE_HPP

// tests/format/src/format_ifdef_input.cpp
#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((noinline, flatten))

// tests/format/src/format_ifdef_input.cpp
#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((always_inline, flatten))

// tests/format/src/format_ifdef_input.cpp
#define USERVER_IMPL_NODEBUG __attribute__((__nodebug__))

// tests/format/src/format_ifdef_input.cpp
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__nodebug__, __always_inline__))

// tests/format/src/format_ifdef_input.cpp
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__always_inline__))

// tests/format/src/format_ifdef_input.cpp
#define FORMAT_USERVER_CONST

// tests/format/src/format_ifdef_input.cpp
#define FORMAT_USERVER_CONST const

// tests/format/src/format_ifdef_input.cpp
#define CURL_FORMAT_USERVER_NAMESPACE fixture::

// tests/format/src/format_ifdef_input.cpp
#define CURL_FORMAT_USERVER_NAMESPACE

// tests/format/src/format_non_ascii_input.cpp
#define СООБЩЕНИЕ Use("01234567890123456789é𝄞")

// tests/format/src/format_optimization_input.cpp
#define JOIN a + "" + b + c

// tests/format/src/format_preprocessor_eof_input.cpp
#define FORMAT_EOF_VALUE 1

// tests/format/src/format_preprocessor_eof_input.cpp
#define FORMAT_EOF_PAIR (sizeof("prefix") - 1, sizeof(">") - 1)

// tests/format/src/format_preprocessor_eof_input.cpp
#define FORMAT_EOF_BODY() do {} \
    while (false)
// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_SUM(firstValue, secondValue, thirdValue) \
    ((firstValue) + (secondValue) + (thirdValue) + (firstValue) + (secondValue) + (thirdValue))

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_SHORT_MACRO(value) (value)

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_MUCH_LONGER_MACRO(value) (value)

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_STATEMENT_ARGUMENT(MethodName, ParamType, ParamName, ...) \
    auto& MethodName(ParamType ParamName) { __VA_ARGS__ return *this; }

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_STRUCTURED_LAMBDA \
    []() {                               \
        a();                             \
        b();                             \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_DECLARE_OPTION(name, type) void name(type)

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_LOAD_OPTIONAL(function, name) \
    function = reinterpret_cast<decltype(function)>(GetProcAddress(module_, name))

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_ITEMS(X) \
    X(Alpha, "alpha")           \
    X(Beta, "beta")             \
    X(Gamma, "gamma")

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_ENUM_ITEMS(X) \
    X(First, "first")                \
    X(Second, "second")

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_COMMENT_CONTINUATION(callback) \
    callback();                                       \
    /* cold testing path: */                          \
    callback();

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_TOKEN_PASTE(prefix, suffix) prefix##suffix

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_FN(name) inline int Get##name##Value() { return 0; }

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_INIT(name) {(name), Get##name##Value()}

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_NUMBER(suffix) 10##suffix

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_STRINGIZE(value) \
    #value

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_FILEPATH FORMAT_NAMESPACE::logging::impl::CutFilePath(__builtin_FILE())

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_REGISTER_TYPE(Type, Index)                                          \
    constexpr std::size_t TypeToId(FormatFixtureIdentity<Type>) noexcept { return Index; } \
    constexpr Type IdToType(FormatFixtureSize<Index>) noexcept { return FormatFixtureConstruct<Type>(); }

// tests/format/src/format_test_input.cpp
#define ENUM_STRING_DECLARE(EnumType, ItemsMacro)                                                              \
    enum class EnumType {                                                                                      \
        ItemsMacro(ENUM_STRING_DECLARE_ENUMERATOR),                                                            \
    };                                                                                                         \
    template <>                                                                                                \
    struct EnumStringTraits<EnumType> {                                                                        \
        static constexpr auto names = std::to_array<std::string_view>({ItemsMacro(ENUM_STRING_DECLARE_NAME)}); \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_TEMP_MACRO(value) (value)

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_METHOD_MARKER(value) (value)

// tests/format/src/format_test_input.cpp
#define COMMENTED_ARGUMENTS(value) Configure(/* mode = */ "default", /* options = */ {}, /* value = */ value)

// tests/format/src/format_test_input.cpp
#define COMMENTED_SUM(first, second) \
    (                                \
        first + /* first term */     \
        second                       \
    )

// tests/format/src/format_test_input.cpp
#define COMMENTED_STREAM(out, value) \
    out /* insertion */              \
        << value

// tests/format/src/format_test_input.cpp
#define FORMAT_NAMESPACE_TRAITS(Type) \
    namespace format_macro {          \
                                      \
    namespace detail {                \
                                      \
    template <>                       \
    struct Traits<Type> {             \
        using type = Type;            \
    };                                \
                                      \
    }                                 \
                                      \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_NESTED_NAMESPACE_TRAITS(Type) \
    namespace format_macro::nested {         \
                                             \
    inline namespace version {               \
                                             \
    template <>                              \
    struct Traits<Type> {                    \
        static_assert(Check<Type>());        \
    };                                       \
                                             \
    }                                        \
                                             \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MIXED_NAMESPACE_DECLARATIONS(Type) \
    namespace format_macro {                      \
                                                  \
    Type Get();                                   \
                                                  \
    }                                             \
    void After();

// tests/format/src/format_test_input.cpp
#define FORMAT_ANONYMOUS_NAMESPACE(Type) \
    namespace {                          \
                                         \
    namespace detail {                   \
                                         \
    Type value;                          \
                                         \
    }                                    \
                                         \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_ALIGN_LONG_LINE() \
    void LongMacroLine() {       \
        Use("This indivisible string literal deliberately exceeds the configured column limit and must not push the other continuation backslashes to the right."); \
        Short();                 \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_ALIGN_RAW_STRING()                \
    void RawMacroLine() {                        \
        Use(R"text(raw string content ending in a backslash \
this line is still inside the raw string)text"); \
        Short();                                 \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_PRIMITIVE_DECLARATION int value;

// tests/format/src/format_test_input.cpp
#define FORMAT_PRIMITIVE_DECL_SEQUENCE \
    int first;                         \
    unsigned long second;              \
    T named;

// tests/format/src/format_test_input.cpp
#define FORMAT_PRIMITIVE_FUNCTIONS \
    void F();                      \
    int G(int value);

// tests/format/src/format_test_input.cpp
#define FORMAT_RECURSIVE_PRIMITIVE_FUNCTIONS \
    int (*Factory())();                      \
    int (&Array())[3];

// tests/format/src/format_test_input.cpp
#define FORMAT_PRIMITIVE_INITIALIZER static const int value = Make();

// tests/format/src/format_test_input.cpp
#define FORMAT_DECLARATION_NAMESPACE_SEQUENCE \
    void Before();                            \
    namespace format_macro {                  \
                                              \
    int value;                                \
                                              \
    }                                         \
    void After();

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_PROTOTYPES(type)     \
    detail::Value First(const type& value);   \
    ::QualifiedMacroFunctions::detail::Value& \
        operator<<(::QualifiedMacroFunctions::detail::Value& value, const type& arg);

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_SINGLE detail::Value First() { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_SEQUENCE         \
    detail::Value Second() { return {}; } \
    int Third() { return 3; }             \
    detail::Value Fourth() { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_CONSTEXPR                 \
    constexpr detail::Value Fifth() { return {}; } \
    constexpr int Sixth() { return 6; }            \
    constexpr detail::Value Seventh() { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_MODIFIERS \
    [[nodiscard]] inline const detail::Value& Ref(const detail::Value& value) { return value; }

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_TEMPLATE \
    template <class T>            \
    detail::Value Convert(T value) { return {static_cast<int>(value)}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_CONSTANT constexpr detail::Value constant{};

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_VARIABLE_NAME(name) name

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_HEADER static detail::Value FORMAT_QUALIFIED_VARIABLE_NAME(variable)

// tests/format/src/format_test_input.cpp
#define FORMAT_QUALIFIED_RECURSIVE detail::Value (*Factory())() { return First; }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_NESTED_DECLARATORS \
    int (((*Factory())))(int);          \
    int (((&Array())))[3];

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_IF(value) \
    if (value) {               \
        result += value;       \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_FOR(values) \
    for (auto value : values) {  \
        result += value;         \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_BLOCK(value) \
    {                             \
        int local = value;        \
        result += local;          \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_MIXED(value) \
    int local = value;            \
    if (local) {                  \
        result += local;          \
    }                             \
    ++result;

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_WHILE(value) \
    while (value > 0) {           \
        result += value;          \
        --value;                  \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_SWITCH(value) \
    switch (value) {               \
        case 1:                    \
            ++result;              \
            break;                 \
        default:                   \
            result += 2;           \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_DO(value) \
    do {                       \
        result += value;       \
    } while (false)

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_UNBRACED_DO(value) \
    do {                                \
        result += value;                \
    } while (false)

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_COMPLETE_DO(value) \
    do {                                \
        result += value;                \
    } while (false);

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_FINAL_DO(value) \
    ++result;                        \
    do {                             \
        result += value;             \
    } while (false)

// tests/format/src/format_test_input.cpp
#define FORMAT_MACRO_TRY(value) \
    try {                       \
        throw value;            \
    } catch (int amount) {      \
        result += amount;       \
    }

// tests/format/src/format_test_input.cpp
#define SELECT_VALUE(record, field) ((record).Has##field() ? (record).field : k_##field)

// tests/format/src/format_test_input.cpp
#define READ_VALUE(record, field) ((record)->get_##field())

// tests/format/src/format_test_input.cpp
#define ASSIGN_VALUE(record, field, result) (record).field##ue = result

// tests/format/src/format_test_input.cpp
#define CONVERT_VALUE(record, field, Type) ((record).template as_##field<Type>())

// tests/format/src/format_test_input.cpp
#define CHAIN_VALUE(record, part, rest) ((record).get_##part##rest())

// tests/format/src/format_test_input.cpp
#define COMPUTE_VALUE(field) (k_##field + 2 * k_##field)

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_FUNCTION(Suffix) constexpr int Get##Suffix() { return 7; }

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_VARIABLE(Suffix) constexpr int k##Suffix = 9;

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_RESULT(Suffix) Result##Suffix Build##Suffix() { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_QUALIFIED(Suffix) Group##Suffix::Result Qualified##Suffix() { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_TEMPLATE(Suffix) \
    template <class T>                      \
    T Convert##Suffix(T value) { return value; }

// tests/format/src/format_test_input.cpp
#define FORMAT_PASTED_DECL_POINTER(Suffix) int (*Pointer##Suffix())(int) { return &Convert##Suffix<int>; }

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_DECLARATIONS(Name)   \
    struct Name##Tag {};                 \
    class Name##Forward;                 \
    union Name##Union {                  \
        int value;                       \
        double other;                    \
    };                                   \
    using Name = Name##Tag;              \
    typedef Name Name##Alias;            \
    using Name##Callback = int (*)(int); \
    using Name##Function = int(int);

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_MEMBER_ALIAS(Name) using Name = int Owner::*;

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_NAMESPACE(Name) \
    namespace Name {                \
                                    \
    using Number = int;             \
                                    \
    }                               \
    namespace Name##Alias = Name;   \
    using Name::Number;

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_TEMPLATE(Name) \
    template <class T>             \
    struct Name {                  \
        T value;                   \
    };                             \
    template <class T>             \
    using Name##Alias = Name<T>;

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_CONCEPT(Name) \
    template <class T>            \
    concept Name = sizeof(T) > 0;

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_EXTERN(Name) \
    extern "C" {                 \
                                 \
    int Name(int);               \
                                 \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_INSTANTIATION(Name) template struct Name<int>;

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_CLASS(Name)                      \
    class Name##Derived final : public Owner {       \
public:                                              \
        Name##Derived(int value) { member = value; } \
    };

// tests/format/src/format_test_input.cpp
#define FORMAT_STATEMENT_PREFIX_THROW throw

// tests/format/src/format_test_input.cpp
#define FORMAT_STATEMENT_PREFIX_DISCARD (void)

// tests/format/src/format_test_input.cpp
#define FORMAT_STATEMENT_PREFIX_FOR(count) for (int index = 0; index < count; ++index)

// tests/format/src/format_test_input.cpp
#define FORMAT_STATEMENT_PREFIX_TRACE() if (true)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_INC(value) ++value;

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_RETURN(value) return value;

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_DECLARE(name) constexpr int name = 2;

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_RELAY(value) \
    FORMAT_SEMILESS_INC(value)       \
    FORMAT_SEMILESS_INC(value)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_TEXT() "part"

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_CONCAT() FORMAT_SEMILESS_TEXT() FORMAT_SEMILESS_TEXT()"end"

// tests/format/src/format_test_input.cpp
#define FORMAT_ANON_ENUM(name) \
    enum : bool {              \
        name = true,           \
    };

// tests/format/src/format_test_input.cpp
#define FORMAT_ANON_ATTR_ENUM(name)    \
    enum [[maybe_unused]] : unsigned { \
        name = 3,                      \
    };

// tests/format/src/format_test_input.cpp
#define FORMAT_ENUM_ATTR(Name)              \
    enum Name {                             \
        Old##Name [[deprecated]],           \
        Current##Name [[maybe_unused]] = 1, \
    };

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_STRINGS "one", "two", "three"

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_VALUES(value) 1, (value), 3, ((value) + 1)

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_TRAILING(value) (value), ((value) + 1),

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_EXPRESSIONS(flag, value) (flag) ? (value) : 1, (value) + 2

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_SINGLE(value) (value),

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_PARENTHESIZED(value) ((value), ((value) + 1))

// tests/format/src/format_test_input.cpp
#define FORMAT_COMMA_QUALIFIED data::Text, data::Box<int>, data::Box<data::Box<bool>>

// tests/format/src/format_test_input.cpp
#define FORMAT_DECLARATOR_MODIFIER

// tests/format/src/format_test_input.cpp
#define FORMAT_DECLARATOR_ATTRIBUTE(...)

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_FIELD(name, value) .name = (value)

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_FIELDS(first_value, second_value) .first = (first_value), .second = (second_value)

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_BRACED(name, ...) .name{__VA_ARGS__}

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_RECORDS(value) {(value), (value) + 1}, {(value) + 2, (value) + 3},

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_NESTED(value) .pair = {FORMAT_INITIALIZER_FIELDS(value, (value) + 1)},

// tests/format/src/format_test_input.cpp
#define FORMAT_INITIALIZER_PASTE(name, value) .name##st = (value),

// tests/format/src/format_test_input.cpp
#define FORMAT_ATTRIBUTE_GNU_INLINE __attribute__((always_inline, pure))

// tests/format/src/format_test_input.cpp
#define FORMAT_ATTRIBUTE_GNU_COLD __attribute__((noinline)) __attribute__((cold))

// tests/format/src/format_test_input.cpp
#define FORMAT_ATTRIBUTE_GNU_ALTERNATE __attribute((unused))

// tests/format/src/format_test_input.cpp
#define FORMAT_ATTRIBUTE_MIXED [[maybe_unused]] __attribute__((unused))

// tests/format/src/format_test_input.cpp
#define FORMAT_ATTRIBUTE_MS_NOINLINE __declspec(noinline)

// tests/format/src/format_test_input.cpp
#define FORMAT_TOKEN_ITEM(name, ...) static constexpr int name = 1;

// tests/format/src/format_test_input.cpp
#define FORMAT_TOKEN_STATEMENT(...) Consume(1);

// tests/format/src/format_test_input.cpp
#define FORMAT_TOKEN_WRAPPER(name, ...) FORMAT_TOKEN_ITEM(name, __VA_ARGS__)

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX [[maybe_unused]]

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX_CALL(...) [[maybe_unused]]

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_METHOD(Return, Name, Parameters, Qualifiers)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_RECORD_FUNCTION(Name) auto Name() -> struct Record { return {}; }

// tests/format/src/format_test_input.cpp
#define FORMAT_STATEMENT_DECLARATIONS(Statement, Tag) \
    do {                                              \
        Statement;                                    \
    } while (false)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_SIZE(Type) sizeof(Type)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_WORDS(...) ((void)0)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_ALIAS(Name, ...) using Name = __VA_ARGS__;

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_VALUE 1

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_VALUE 2

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_VALUE 3

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_VALUE 4

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_OPERAND 2

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_PARAMETER Integer

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_ARGUMENT 3

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_ARGUMENT 1

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_RAW_TEXT \
    R"tag(
#define UNEXPANDED 3
#undef UNEXPANDED
)tag"

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_RECORD(name) \
    struct name {                \
        using Value = int;       \
                                 \
        Value value;             \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_CHOICE(name) \
    union name {                 \
        int number;              \
        char letter;             \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_ENUM(name)   \
    enum class name : unsigned { \
        First,                   \
        Second,                  \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_PRIMITIVE_TYPE unsigned long

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_TAG \
    struct NamedType {  \
        int value;      \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_NAMESPACE_COMMENT() \
    namespace Outer {                       \
                                            \
    namespace Inner {                       \
                                            \
    int value = 1;                          \
                                            \
    } /* Inner */                           \
                                            \
    } /* Outer */

// tests/format/src/format_test_input.cpp
#define FORMAT_LAMBDA_CALLBACK(value)                                                         \
    do {                                                                                      \
        Invoke(                                                                               \
            "a callback argument that makes the invocation exceed the configured line width", \
            value,                                                                            \
            [](int argument) {                                                                \
                if (argument) {                                                               \
                    ++argument;                                                               \
                }                                                                             \
            }                                                                                 \
        );                                                                                    \
    } while (false)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_GENERATE(X) \
    X(First, 2)                     \
    X(Second, 3)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_EMPTY(X)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_SINGLE(X) X(Third, 4)

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_ENUM(name, value) name = value,

// tests/format/src/format_test_input.cpp
#define FORMAT_LIST_ENTRY(name, value) value,

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX_LIST 6, 7,

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX_EMPTY

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX_VALUE 9

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_VALUE() 10

// tests/format/src/format_test_input.cpp
#define FORMAT_PARAMETER_SUFFIX_ENUM One, Two,

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_TEXT() "x"

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_GENERATED(name)  \
    enum class name {                \
        FORMAT_PARAMETER_SUFFIX_ENUM \
        Last,                        \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_FIELD int first = 1;

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_EXTRA int second = 2;

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_EMPTY

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_COMBINED \
    FORMAT_BARE_FIELD        \
    FORMAT_BARE_EXTRA

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_UNION_FIELD int number;

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_DECLARE_GENERATED(Type) \
    struct Type {                               \
        FORMAT_BARE_COMBINED                    \
        int last = 3;                           \
    };

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_INCREMENT(Value) ((Value) + 1)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_FORWARD_BARE(Value) FORMAT_BARE_INCREMENT(Value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_TEMPLATE_HEADER(Name) \
    template <class T>                    \
    T Name(T value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_SPECIALIZED_HEADER(Name) \
    template <>                              \
    int Name<int>(int value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_NESTED_HEADER() \
    template <class T>              \
    template <class U>              \
    U Values<T>::Convert(U value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_CONSTRAINED_HEADER(Name)   \
    template <class T> requires(sizeof(T) > 0) \
    T Name(T value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_CONSTRAINED_SUFFIX(Name) \
    template <class T>                       \
    T Name(T value) requires(sizeof(T) > 0)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_REFERENCE_HEADER(Name) \
    template <class T>                     \
    T& Name(T& value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_FACTORY_HEADER(Name) \
    template <class T>                   \
    T (*Name())(T)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_TRAILING_HEADER(Name) \
    template <class T>                    \
    auto Name(T* value) -> T*

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_ARRAY_HEADER(Name) \
    template <class T, unsigned N>     \
    T (&Name(T (&value)[N]))[N]

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_VOID_HEADER(Name) \
    template <class T>                \
    void Name(const T&)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_SEQUENCE_HEADER(Name) \
    struct Tag {};                        \
    using Alias = Tag;                    \
    void Declared();                      \
    template <class T>                    \
    T Name(T value)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_VARIABLE(Name) \
    template <class T>             \
    constexpr T Name = 42

// tests/format/src/format_test_input.cpp
#define FORMAT_JOIN_INNER(Left, Right) Left##Right

// tests/format/src/format_test_input.cpp
#define FORMAT_JOIN(Left, Right) FORMAT_JOIN_INNER(Left, Right)

// tests/format/src/format_test_input.cpp
#define FORMAT_TYPE_TRACE(Message) \
    const ::TemplateHeaderMacros::Details::Trace FORMAT_JOIN(trace_, __LINE__)(__FILE__, __LINE__, (Message))

// tests/format/src/format_test_input.cpp
#define FORMAT_GUARDED_NAMESPACE_ENABLED 1

// tests/format/src/format_test_input.cpp
#define FORMAT_GUARDED_NAMESPACE_SEEN

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_LOGICAL_TAIL(Left, Right) &&((Left) == (Right))

// tests/format/src/format_test_input.cpp
#define FORMAT_FIXTURE_ARITHMETIC_TAIL(Value) +(Value)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_COMPARE(Left, Right) FORMAT_FIXTURE_LOGICAL_TAIL(Left, Right)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_ADD(Value) FORMAT_FIXTURE_ARITHMETIC_TAIL(Value)

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_TRUE_TAIL FORMAT_SEMILESS_COMPARE(1, 1)

// tests/format/src/format_test_input.cpp
#define FORMAT_TOKEN_COMPARE(Callback, Field) Callback(a.Field, b.Field)

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_EQUAL(Type)                                                            \
    inline bool operator==(const Type& a, const Type& b) noexcept {                            \
        return true FORMAT_TOKEN_COMPARE(FORMAT_SEMILESS_COMPARE, first) FORMAT_TOKEN_COMPARE( \
            FORMAT_SEMILESS_COMPARE, second                                                    \
        );                                                                                     \
    }

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_LIST_VALUES 3, 4,

// tests/format/src/format_test_input.cpp
#define FORMAT_BARE_LIST_EMPTY

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_LIST_VALUES() 5, 6,

// tests/format/src/format_test_input.cpp
#define FORMAT_SEMILESS_LIST_EMPTY()

// tests/format/src/format_userver_input.cpp
#define FORMAT_USERVER_DO_WHILE(flag) \
    do {                              \
        if (flag) {                   \
            break;                    \
        }                             \
        UseFlag(flag);                \
    } while (false)

// tests/format/src/format_userver_input.cpp
#define USERVER_IMPL_FORCE_INLINE __attribute__((always_inline)) inline

// tests/format/src/format_userver_input.cpp
#define LOG_FORMAT_USERVER_LIMITED(logger, level, ...)     \
    if (const RateLimiter limiter{[]() -> RateLimitData& { \
            static RateLimitData data;                     \
            return data;                                   \
        }()};                                              \
        !limiter.ShouldLog())                              \
    {                                                      \
    } else                                                 \
        LOG_TO((logger), (level), __VA_ARGS__) << limiter

// tests/format/src/format_userver_input.cpp
#define FORMAT_USERVER_COMPLEX_OPTION(FUNCTION_NAME, OPTION_TYPE) \
    inline void FUNCTION_NAME(OPTION_TYPE arg) { UseOption(arg, PP_STRINGIZE(FUNCTION_NAME)); }

// tests/format/src/format_userver_input.cpp
#define FORMAT_USERVER_HASH_JOIN(FUNCTION_NAME) \
    private:                                    \
        inline void FUNCTION_NAME##_impl() {}   \
    public:                                     \
        static constexpr bool is_##FUNCTION_NAME##_available = true

// tests/format/src/format_userver_input.cpp
#define FORMAT_USERVER_EXPECT_TRY(cmd)                 \
    try {                                              \
        cmd;                                           \
    } catch (const Error& error) {                     \
        EXPECT_EQ(error.Code(), ErrorCode::kExpected); \
    }

// tests/format/src/format_userver_input.cpp
#define BENCHMARK_THREAD_ARGS ->Arg(2)->Arg(4)

// tests/format/src/format_userver_input.cpp
#define IMPL_UTEST_FORMAT_USERVER(name)             \
    TestLauncher<::testing::Test>::RunTest<name>(); \
    struct FormatUserverForceSemicolon

// tests/format/src/format_userver_input.cpp
#define FORMAT_EMPTY_TEST(suite, name) void suite()

// tests/format/src/format_userver_input.cpp
#define FORWARD_EMPTY_ARGUMENTS(value) TEST_COMMAND(value,, /* empty */)

// tests/format/src/format_userver_input.cpp
#define NESTED_EMPTY_ARGUMENTS(value) TEST_COMMAND(, TEST_COMMAND(value,, ), TEST_COMMAND(TEST_COMMAND(, )), )

// tests/format/src/format_userver_input.cpp
#define FORMAT_CALL_MACRO_ALIAS TEST_COMMAND

// tests/format/src/format_userver_input.cpp
#define FORMAT_USERVER_COMPLETE_STATEMENT(value) ++value;

// Raw continuation alignment preserves fragments and each definition's own alignment group.
#define FORMAT_RAW_ALIGN_MEMBERS(T) \
    private:                        \
        T& Borrow(Owner& owner);    \
    public:                         \
        using Base::Base

#define FORMAT_RAW_ALIGN_OVER_LIMIT() \
    )                                 \
    ThisIndivisiblePreprocessingTokenIsIntentionallyLongerThanTheColumnLimitAndMustNotPushOtherContinuationBackslashesPastTheirOwnAlignmentColumn \
                                      \
    short_value                       \
    ThisFinalLineIsAlsoDeliberatelyLongerThanTheColumnLimitAndMustNotParticipateInTheAlignmentOfAnyPrecedingContinuationBackslashes

// Literal-internal backslashes and indentation stay intact; surrounding continuations align.
#define FORMAT_RAW_ALIGN_LITERALS()       \
    ) u8R"tag(first literal line \
                another literal line)tag" \
    "first string line\
second string line"                       \
    last_value

// Preserve token-joining splices while aligning independent continuation lines.
#define FORMAT_RAW_ALIGN_TOKEN_JOIN() \
    namespace Joined\
Name {                                \
    value +\
+;                                    \
    public:

// Quotes in a continued comment do not start literals.
#define FORMAT_RAW_ALIGN_COMMENT() \
    )  // " and ' are comment text \
        continued comment text

// Punctuation separated by splices still aligns when the source omits padding.
#define FORMAT_RAW_ALIGN_NO_SPACE() \
    )                               \
    {                               \
    value;                          \
    }                               \
    end
