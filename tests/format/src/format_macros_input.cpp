// Macro replacements extracted from services and existing golden fixtures.
// Identical definitions are represented once.

#define ADP_ADD_TO_DRAFT_DIFF(field)   \
    if (body.field) {                  \
        diff.new_.field = *body.field; \
    }

#define RATE_COUNTER(name)                                                            \
public:                                                                               \
    void Add##name(size_t count = 1) { name##_ += ::utils::statistics::Rate{count}; } \
    auto Get##name() const { return name##_; }                                        \
                                                                                      \
private:                                                                              \
    ::utils::statistics::RateCounter name##_{};

#define API_PATH() bike::management::utils::GetApiPath(__FILE__)

#define ADD_PG_CONVERTER(converter)                                                               \
    namespace storages::postgres::io {                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct CppToSystemPg<converter::UserType> : CppToSystemPg<converter::PostgresType> {};        \
                                                                                                  \
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

#define AUTO_FIELD(name, ...) std::remove_reference_t<decltype(__VA_ARGS__)> name = (__VA_ARGS__)

#define REQUIREMENT(...) static_cast<void>(0)

#define CURRENT_SOURCE_ROOT "taxi/uservices/services/candidates"

#define CONTEXT_DETAIL(context, detail)           \
    do {                                          \
        if ((context).NeedDetails()) [[unlikely]] \
            (context).AddDetail((detail));        \
    } while (0)

#define CONTEXT_DETAIL_FMT(context, ...) CONTEXT_DETAIL((context), std::format(__VA_ARGS__))

#define LOG(level)                                  \
    level < candidates::sdk::logging::GetLogLevel() \
        ? candidates::sdk::logging::Noop{}          \
        : candidates::sdk::logging::LogHelper(level).AsLvalue()

#define LOG_ERROR() LOG(userver::logging::Level::kError)

#define LOG_WARNING() LOG(userver::logging::Level::kWarning)

#define LOG_INFO() LOG(userver::logging::Level::kInfo)

#define LOG_DEBUG() LOG(userver::logging::Level::kDebug)

#define LOG_TRACE() LOG(userver::logging::Level::kTrace)

#define INVARIANT(condition, message)                               \
    do {                                                            \
        if (condition) {                                            \
        } else {                                                    \
            interop::Log(userver::logging::Level::kError, message); \
            std::abort();                                           \
        }                                                           \
    } while (0)

#define THROW_SERVER_ERROR_MESSAGE(code, message, log_extra)                              \
    {                                                                                     \
        const logging::LogExtra& log_extra_value = log_extra;                             \
        LOG_ERROR("{} | {}", code, message) << log_extra_value;                           \
        throw ErrorMessageException(ErrorMessageException::Type::kServer, code, message); \
    }

#define THROW_CLIENT_ERROR_MESSAGE(code, message, log_extra)                              \
    {                                                                                     \
        const logging::LogExtra& log_extra_value = log_extra;                             \
        LOG_ERROR("{} | {}", code, message) << log_extra_value;                           \
        throw ErrorMessageException(ErrorMessageException::Type::kClient, code, message); \
    }

#define SENSITIVE_DATA_MASKING_STRINGIZE_AND_COMMA(r, data, elem) \
    ::sensitive_data_masking::PathElementHelper(BOOST_PP_STRINGIZE(elem)),

#define SENSITIVE_DATA_MASKING_UNWRAP_SEQ(s, state, x) (::sensitive_data_masking::UnwrapHelper(state).x)

#define SENSITIVE_DATA_MASKING_MAKE_JSON_PATH_SEQ(prefix, seq)                                             \
    (::std::is_void_v<decltype(BOOST_PP_SEQ_FOLD_LEFT(SENSITIVE_DATA_MASKING_UNWRAP_SEQ, (prefix), seq))>, \
     ::sensitive_data_masking::JsonPath{BOOST_PP_SEQ_FOR_EACH(SENSITIVE_DATA_MASKING_STRINGIZE_AND_COMMA, , seq)})

#define MAKE_JSON_PATH(prefix, ...) \
    SENSITIVE_DATA_MASKING_MAKE_JSON_PATH_SEQ(prefix, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define LOCALIZE(key) result.title_##key = utils_wb::TranslateForWebConstructor(kCurrentTabPrefix + #key, localizer);

#define EXTRACT_2(opt, member) ((opt)->*&DecayValueType<decltype(opt)>::value_type::member)

#define EXTRACT_3(opt, m1, m2) EXTRACT_2(EXTRACT_2(opt, m1), m2)

#define EXTRACT_4(opt, m1, m2, m3) EXTRACT_2(EXTRACT_3(opt, m1, m2), m3)

#define EXTRACT_5(opt, m1, m2, m3, m4) EXTRACT_2(EXTRACT_4(opt, m1, m2, m3), m4)

#define EXTRACT_6(opt, m1, m2, m3, m4, m5) EXTRACT_2(EXTRACT_5(opt, m1, m2, m3, m4), m5)

#define EXTRACT_7(opt, m1, m2, m3, m4, m5, m6) EXTRACT_2(EXTRACT_6(opt, m1, m2, m3, m4, m5), m6)

#define GET_8TH_ARG(arg1, arg2, arg3, arg4, arg5, arg6, arg7, N, ...) N

#define EXTRACT_MACRO_CHOOSER(...) \
    GET_8TH_ARG(__VA_ARGS__, EXTRACT_7, EXTRACT_6, EXTRACT_5, EXTRACT_4, EXTRACT_3, EXTRACT_2)

#define EXTRACT_MEMBER(opt, ...)            \
    EXTRACT_MACRO_CHOOSER(opt, __VA_ARGS__) \
    (ExtractStartTag{}->*(opt), __VA_ARGS__)

#define EXTRACT_REF(opt, ...)               \
    EXTRACT_MACRO_CHOOSER(opt, __VA_ARGS__) \
    (ExtractRefStartTag{}->*(opt), __VA_ARGS__)

#define NAMED_FIELD(field) ::cargo_performer_self_assignment::proposal::app::filters::NamedField(field, #field)

#define MOCK_RESPONSE(dependencies, ResponseBodyType)                                                                  \
    const auto handler_path_opt = cargo_performer_self_assignment::helpers::mock_response::ExtractHandlerPath(__FILE__ \
    );                                                                                                                 \
    if (handler_path_opt.has_value()) {                                                                                \
        const auto mock = cargo_performer_self_assignment::helpers::mock_response::GetMockResponse(                    \
            dependencies,                                                                                              \
            handler_path_opt.value()                                                                                   \
        );                                                                                                             \
        if (mock.response_body.has_value()) {                                                                          \
            LOG_WARNING() << "Mock response for " << handler_path_opt.value();                                         \
            return Response200{Parse(mock.response_body.value().extra, formats::parse::To<ResponseBodyType>())};       \
        }                                                                                                              \
    }

#define CARGO_PRICING_TRANSFORM_AND_LOCATION(transform_name)                                               \
    Output transform_name(Output output, const Input& input);                                              \
                                                                                                           \
    inline cargo_pricing::internal::transform::PathToTransformSourceFile Get##transform_name##Location() { \
        return cargo_pricing::internal::transform::BuildPathToTransformSouceFile(__FILE__);                \
    }

#define CARGO_PRICING_REGISTER_TRANSFORM(transform_name, version) \
    {(transform_name), (version), Get##transform_name##Location()}

#define CARGO_PRICING_IREADREPOSITORY_INCLUDE(PricerIReadRepository) \
    using PricerIReadRepository::Load;                               \
    using PricerIReadRepository::LoadMany;                           \
    using PricerIReadRepository::TryLoad;                            \
    using PricerIReadRepository::TryLoadMany;

#define CARGO_PRICING_IWRITEREPOSITORY_INCLUDE(PricerIWriteRepository) \
    using PricerIWriteRepository::Save;                                \
    using PricerIWriteRepository::SaveCalcMetadata;

#define CARGO_PRICING_IRWREPOSITORY_INCLUDE(PricerIRepository) \
    CARGO_PRICING_IREADREPOSITORY_INCLUDE(PricerIRepository)   \
    CARGO_PRICING_IWRITEREPOSITORY_INCLUDE(PricerIRepository)

#define AMO_CALL(NAMESPACE, HANDLER, should_set_auth_at_start)                                                       \
    const auto& cluster = deps.pg_cargo_sf->GetCluster();                                                            \
    const auto& amo_secdist = deps.extra.amo_secdist;                                                                \
    const auto& auth_url = cargo_sf::utils::GetAmoAuthUrl(deps);                                                     \
                                                                                                                     \
    const auto& amo_domain_url = cargo_sf::utils::GetAmoCompanyDomainUrl(deps, domain);                              \
    const auto& amocrm_cargo_client = deps.extra.amocrm_cargo_component.GetClientFor(amo_domain_url.url);            \
    if (should_set_auth_at_start) {                                                                                  \
        request.authorization = cargo_sf::utils::GetAmoAuthTokenFromDb(cluster, amo_secdist, domain);                \
    }                                                                                                                \
                                                                                                                     \
    try {                                                                                                            \
        return amocrm_cargo_client.HANDLER(request);                                                                 \
    } catch (const NAMESPACE::Response401&) {                                                                        \
        LOG_INFO() << "Refreshing auth token due to 401 response";                                                   \
        request.authorization =                                                                                      \
            cargo_sf::utils::GetAmoAutTokenWithRefresh(cluster, amo_secdist, auth_url, amocrm_cargo_client, domain); \
        return amocrm_cargo_client.HANDLER(std::move(request));                                                      \
    }

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#define TAXI_CONSENT_TYPE_FROM_STRING(name, str) \
    if (tag_str == str) return TaxiConsentType::name;

#define TAXI_CONSENT_TYPE_TO_STRING(name, str) \
    case TaxiConsentType::name:                \
        return str;

#define TAXI_CONSENT_TYPES(X) \
    X(kAfisha,                              "afisha")                              \
    X(kBerizaryad,                          "berizaryad")                          \
    X(kBuyandsellCis,                       "buyandsell_cis")                      \
    X(kCare,                                "care")                                \
    X(kChargeAndGo,                         "charge_and_go")                       \
    X(kChargersLowBatteryLocalNotification, "chargers-low-battery-local-notification") \
    X(kDelivery,                            "delivery")                            \
    X(kDrive,                               "drive")                               \
    X(kFuel,                                "fuel")                                \
    X(kHomeServices,                        "home_services")                       \
    X(kIntercity,                           "intercity")                           \
    X(kMaps,                                "maps")                                \
    X(kMarket,                              "market")                              \
    X(kMarketing,                           "marketing")                           \
    X(kNavi,                                "navi")                                \
    X(kNewFeature,                          "new_feature")                         \
    X(kOtherServices,                       "other_services")                      \
    X(kPartners,                            "partners")                            \
    X(kPay,                                 "pay")                                 \
    X(kPersonalGoals,                       "personal_goals")                      \
    X(kPharmacy,                            "pharmacy")                            \
    X(kPlaces,                              "places")                              \
    X(kPlus,                                "plus")                                \
    X(kPromotions,                          "promotions")                          \
    X(kRecommendedRide,                     "recommended_ride")                    \
    X(kRentacar,                            "rentacar")                            \
    X(kRestaurants,                         "restaurants")                         \
    X(kScooters,                            "scooters")                            \
    X(kSellAndBuy,                          "sell_and_buy")                        \
    X(kShops,                               "shops")                               \
    X(kTransportMsk,                        "transport_msk")                       \
    X(kTransportOther,                      "transport_other")                     \
    X(kTravel,                              "travel")                              \
    X(kYangoMarket,                         "yango_market")                        \
    X(kYangoTransport,                      "yango_transport")

#define TAXI_CONSENT_TYPE_ENUM(name, str) name,

#define TAXI_CONSENT_TYPE_STR(name, str) str,

#define BOOST_REGEX_MAX_STATE_COUNT 1000000

#define BOOST_REGEX_MAX_BLOCKS 256

#define FIELD(x) \
    FieldWithName { #x, x }

#define COUPONCHECK_TO_STRING_HELPER(value) \
    case CheckExceptionCode::value:         \
        return #value;

#define CHECKER(func) \
    CouponChecker { #func, func }

#define UPDATE_VALUE(field, field_name)                                              \
    UpdateValue(                                                                     \
        values,                                                                      \
        (current_series) ? std::make_optional(current_series->field) : std::nullopt, \
        modified_series.field,                                                       \
        field_name                                                                   \
    )

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

#define COUPONS_EDIT_FIELD(name, value) static constexpr const char* name = value;

#define COUPONS_EDIT_FIELD(name, value) +1

#define COUPONS_EDIT_FIELD(name, value) value,

#define MAKE_DIFINITION_ENUM_TO_STRING(BM, ENUM_NAME)                   \
    std::string EnumToString(ENUM_NAME enum_value) {                    \
        if (BM.right.count(enum_value)) return BM.right.at(enum_value); \
        return "";                                                      \
    }

#define MAKE_DIFINITION_GET_ENUM_FROM_STRING(BM, ENUM_NAME)             \
    ENUM_NAME Get##ENUM_NAME##FromString(std::string enum_string) {     \
        if (BM.left.count(enum_string)) return BM.left.at(enum_string); \
        return ENUM_NAME::kUnknown;                                     \
    }

#define REGISTER_TYPE(NAME, TYPE) \
    struct NAME##Tag {};          \
    using NAME = utils::StrongTypedef<NAME##Tag, TYPE>;

#define APPLY_PROBLEM(cur_problem, code) \
    if (problem != cur_problem) do       \
        {                                \
            code;                        \
    } while (0)

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define LIKELY(x) (x)

#define UNLIKELY(x) (x)

#define EXPECT_BOTH_EQUAL(msg1, msg2)                                                                        \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
        EXPECT_TRUE(our_result) << "Messages should be equal";                                               \
    } while (0)

#define EXPECT_BOTH_NOT_EQUAL(msg1, msg2)                                                                    \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
        EXPECT_FALSE(our_result) << "Messages should NOT be equal";                                          \
    } while (0)

#define EXPECT_PARITY(msg1, msg2)                                                                            \
    do {                                                                                                     \
        bool our_result = proto_equals::DeterministicSerializeEquals(msg1, msg2);                            \
        bool std_result = MessageDifferencer::Equals(msg1, msg2);                                            \
        EXPECT_EQ(our_result, std_result) << "Parity mismatch: our=" << our_result << " std=" << std_result; \
    } while (0)

#define ADD_VALUE(result, proto, field)    \
    if (proto.has_##field()) {             \
        result.field = proto.Get##field(); \
    }

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
    XX(kDisabledPaymentTypePersonalWalletIfNoCashbackPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS" \
    )                                                                                                                  \
    XX(kCoopAccountUnavailable, "COOP_ACCOUNT_UNAVAILABLE")                                                            \
    XX(kAgentPaymentUnavailable, "AGENT_PAYMENT_UNAVAILABLE")                                                          \
    XX(kPersonalWalletInsufficientFunds, "PERSONAL_WALLET_INSUFFICIENT_FUNDS")                                         \
    XX(kComplementUnavailable, "COMPLEMENT_UNAVAILABLE")                                                               \
    XX(kComplementPaymentChanged, "COMPLEMENT_PAYMENT_CHANGED")                                                        \
    XX(kCorpZoneUnavailable, "CORP_ZONE_UNAVAILABLE")                                                                  \
    XX(kCorpCannotOrder, "CORP_CANNOT_ORDER")                                                                          \
    XX(kDestinationZoneRestrictionError, "DESTINATION_ZONE_RESTRICTION_ERROR")                                         \
    XX(kPaymentTypeMismatchesOffer, "PAYMENT_TYPE_MISMATCHES_OFFER")                                                   \
    XX(kCorpPaymentMethodMismatchesOffer, "CORP_PAYMENT_METHOD_MISMATCHES_OFFER")                                      \
    XX(kDoorToDoorCommentNotFilled, "DOOR_TO_DOOR_COMMENT_NOT_FILLED")                                                 \
    XX(kCashRestrictedByTags, "CASH_RESTRICTED_BY_TAGS")                                                               \
    XX(kTariffzoneNotFound, "TARIFFZONE_NOT_FOUND")                                                                    \
    XX(kTariffMultiorderExceeded, "TARIFF_MULTIORDER_EXCEEDED")                                                        \
    XX(kNoUserEmail, "NO_USER_EMAIL")                                                                                  \
    XX(kDeliveryRestricted, "DELIVERY_RESTRICTED")                                                                     \
    XX(kPreorderUnavailable, "PREORDER_UNAVAILABLE")                                                                   \
    XX(kMultiorderDisallowedForSpammer, "MULTIORDER_DISALLOWED_FOR_SPAMMER")                                           \
    XX(kMaasFlowFailed, "MAAS_FLOW_FAILED")                                                                            \
    XX(kAgentApplicationChanged, "AGENT_APPLICATION_CHANGED")                                                          \
    XX(kOrderDraftExpired, "ORDER_DRAFT_EXPIRED")

#define ERROR(x) \
    Error { std::string(x) }

#define AMO_CALL(NAMESPACE, HANDLER, should_set_auth_at_start)                                            \
    const auto& cluster = deps.pg_delivery_bpm->GetCluster();                                             \
    const auto& amo_secdist = deps.extra.amo_secdist;                                                     \
    const auto& auth_url = delivery_bpm::utils::GetAmoAuthUrl(deps);                                      \
                                                                                                          \
    const auto& amo_domain_url =                                                                          \
        delivery_bpm::utils::GetAmoCompanyDomainUrl(deps.config[taxi_config::AMOCRM_CARGO_URL], domain);  \
    const auto& amocrm_cargo_client = deps.extra.amocrm_cargo_component.GetClientFor(amo_domain_url.url); \
    if (should_set_auth_at_start) {                                                                       \
        request.authorization = delivery_bpm::utils::GetAmoAuthTokenFromDb(cluster, amo_secdist, domain); \
    }                                                                                                     \
                                                                                                          \
    try {                                                                                                 \
        return amocrm_cargo_client.HANDLER(request);                                                      \
    } catch (const NAMESPACE::Response401&) {                                                             \
        LOG_INFO() << "Refreshing auth token due to 401 response";                                        \
        request.authorization = delivery_bpm::utils::GetAmoAuthTokenWithRefresh(                          \
            cluster,                                                                                      \
            amo_secdist,                                                                                  \
            auth_url,                                                                                     \
            amocrm_cargo_client,                                                                          \
            domain                                                                                        \
        );                                                                                                \
        return amocrm_cargo_client.HANDLER(std::move(request));                                           \
    }

#define INSTANTIATE_BILATERAL_CONVERSION(FUNC, TYPE1, TYPE2) \
    template TYPE1 FUNC(const TYPE2&);                       \
    template TYPE1 FUNC(TYPE2&&);                            \
    template TYPE2 FUNC(const TYPE1&);                       \
    template TYPE2 FUNC(TYPE1&&);

#define INSTANTIATE_BILATERAL_CONVERSION_HANDLERS_INTERNAL(FUNC, TYPE) \
    INSTANTIATE_BILATERAL_CONVERSION(FUNC, handlers::TYPE, internal_country_specifics::TYPE)

#define ENSURE_RESULT_ACCESS(cond, message, location)    \
    do {                                                 \
        if (!(cond)) {                                   \
            throw(location) + yexception() << (message); \
        }                                                \
    } while (false)

#define BIND(callable, ...) std::bind(callable, std::placeholders::_1, __VA_ARGS__)

#define INJECT_ERROR_IN_TESTSUITE(name)                                                                         \
    do {                                                                                                        \
        TESTPOINT_CALLBACK(                                                                                     \
            std::string("error_injection::") + name,                                                            \
            ::formats::json::Value{},                                                                           \
            [](const ::formats::json::Value& doc) {                                                             \
                if (doc.IsObject() && !doc["inject_faliure"].IsMissing() && doc["inject_faliure"].As<bool>()) { \
                    throw std::runtime_error{"injected error"};                                                 \
                }                                                                                               \
            }                                                                                                   \
        );                                                                                                      \
    } while (false)

#define DECLARE_RESULT(error, ...)                       \
    enum class error##Error : std::uint8_t{__VA_ARGS__}; \
                                                         \
    using error##Result = result::Result<error##Error>

#define DECLARE_RESULT_VALUE(value, error, ...)          \
    enum class error##Error : std::uint8_t{__VA_ARGS__}; \
                                                         \
    using error##Result = result::ResultValue<value, error##Error>

#define MOCK_RESPONSE(dependencies, ResponseBodyType)                                                                 \
    const auto handler_path_opt = delivery_documents::utils::mock_response::ExtractHandlerPath(__FILE__);             \
    if (handler_path_opt.has_value()) {                                                                               \
        const auto                                                                                                    \
            mock = delivery_documents::utils::mock_response::GetMockResponse(dependencies, handler_path_opt.value()); \
        if (mock.response_body.has_value()) {                                                                         \
            LOG_WARNING() << "Mock response for " << handler_path_opt.value();                                        \
            return Response200{Parse(mock.response_body.value().extra, formats::parse::To<ResponseBodyType>())};      \
        }                                                                                                             \
    }

#define NAMED_VALUE(field) GetNamedValue(CutPrefix(#field), field)

#define cimg_use_png

#define CHECK_AND_LOG(a, b, field, ans) \
    if (a.field != b.field) {           \
        ans.push_back(#field);          \
    }

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

#define __CODER__

#define MILE 1.852  // коэфф мили/километры

#define SIZE_TRACKER_FIELD 16

#define BAD_OBJ (-1)

#define SOCKET_BUF_SIZE (4096)

#define MAX_RECORDS (30)

#define MAX_TERMINALS 1000

#define UTS2010 (1262304000)  // unix timestamp 00:00:00 01.01.2010

#define __EGTS__

#define EGTS_PT_RESPONSE 0

#define EGTS_PT_APPDATA 1

#define EGTS_PT_SIGNED_APPDATA 2

#define EGTS_ACK_SERVICE (0)

#define EGTS_AUTH_SERVICE (1)

#define EGTS_TELEDATA_SERVICE (2)

#define EGTS_COMMANDS_SERVICE (4)

#define EGTS_FIRMWARE_SERVICE (9)

#define EGTS_ECALL_SERVICE (10)

#define EGTS_SR_RECORD_RESPONSE 0

#define EGTS_SR_POS_DATA 16

#define EGTS_SR_EXT_POS_DATA 17

#define EGTS_SR_AD_SENSORS_DATA 18

#define EGTS_SR_COUNTERS_DATA 19

#define EGTS_SR_ACCEL_DATA 20

#define EGTS_SR_STATE_DATA 21  // http://forum.gurtam.com/viewtopic.php?pid=48848#p48848

#define EGTS_SR_LOOPIN_DATA 22

#define EGTS_SR_ABS_DIG_SENS_DATA 23

#define EGTS_SR_ABS_AN_SENS_DATA 24

#define EGTS_SR_ABS_CNTR_DATA 25

#define EGTS_SR_ABS_LOOPIN_DATA 26

#define EGTS_SR_LIQUID_LEVEL_SENSOR 27

#define EGTS_SR_PASSENGERS_COUNTERS 28

#define EGTS_SR_DISPATCHER_IDENTITY 5

#define EGTS_SR_EXT_DATA 44

#define EGTS_SR_TERM_IDENTITY (1)

#define EGTS_SR_MODULE_DATA (2)

#define EGTS_SR_VEHICLE_DATA (3)

#define EGTS_SR_AUTH_PARAMS (6)

#define EGTS_SR_AUTH_INFO (7)

#define EGTS_SR_SERVICE_INFO (8)

#define EGTS_SR_RESULT_CODE (9)

#define EGTS_IMEI_LEN 15U

#define EGTS_IMSI_LEN 16U

#define EGTS_LNGC_LEN 3U

#define EGTS_NID_LEN 3U

#define EGTS_MSISDN_LEN 15U

#define EGTS_SR_COMMAND_DATA 51

#define EGTS_FLEET_GET_DOUT_DATA 0x000B

#define EGTS_FLEET_GET_POS_DATA 0x000C

#define EGTS_FLEET_GET_SENSORS_DATA 0x000D

#define EGTS_FLEET_GET_LIN_DATA 0x000E

#define EGTS_FLEET_GET_CIN_DATA 0x000F

#define EGTS_FLEET_GET_STATE 0x0010

#define EGTS_FLEET_ODOM_CLEAR 0x0011

#define EGTS_PC_OK 0                 // успешно обработано

#define EGTS_PC_IN_PROGRESS 1        // в процессе обработки

#define EGTS_PC_UNS_PROTOCOL 128     // неподдерживаемый протокол

#define EGTS_PC_DECRYPT_ERROR 129    // ошибка декодирования

#define EGTS_PC_PROC_DENIED 130      // обработка запрещена

#define EGTS_PC_INC_HEADERFORM 131   // неверный формат заголовка

#define EGTS_PC_INC_DATAFORM 132     // неверный формат данных

#define EGTS_PC_UNS_TYPE 133         // неподдерживаемый тип

#define EGTS_PC_NOTEN_PARAMS 134     // неверное количество параметров

#define EGTS_PC_DBL_PROC 135         // попытка повторной обработки

#define EGTS_PC_PROC_SRC_DENIED 136  // обработка данных от источника запрещена

#define EGTS_PC_HEADERCRC_ERROR 137  // ошибка контрольной суммы заголовка

#define EGTS_PC_DATACRC_ERROR 138    // ошибка контрольной суммы данных

#define EGTS_PC_INVDATALEN 139       // некорректная длина данных

#define EGTS_PC_ROUTE_NFOUND 140     // маршрут не найден

#define EGTS_PC_ROUTE_CLOSED 141     // маршрут закрыт

#define EGTS_PC_ROUTE_DENIED 142     // маршрутизация запрещена

#define EGTS_PC_INVADDR 143          // неверный адрес

#define EGTS_PC_TTLEXPIRED 144       // превышено количество ретрансляции данных

#define EGTS_PC_NO_ACK 145           // нет подтверждения

#define EGTS_PC_OBJ_NFOUND 146       // объект не найден

#define EGTS_PC_EVNT_NFOUND 147      // событие не найдено

#define EGTS_PC_SRVC_NFOUND 148      // сервис не найден

#define EGTS_PC_SRVC_DENIED 149      // сервис запрещён

#define EGTS_PC_SRVC_UNKN 150        // неизвестный тип сервиса

#define EGTS_PC_AUTH_DENIED 151      // авторизация запрещена

#define EGTS_PC_ALREADY_EXISTS 152   // объект уже существует

#define EGTS_PC_ID_NFOUND 153        // идентификатор не найден

#define EGTS_PC_INC_DATETIME 154     // неправильная дата и время

#define EGTS_PC_IO_ERROR 155         // ошибка ввода/вывода

#define EGTS_PC_NO_RES_AVAIL 156     // недостаточно ресурсов

#define EGTS_PC_MODULE_FAULT 157     // внутренний сбой модуля

#define EGTS_PC_MODULE_PWR_FLT 158   // сбой в работе цепи питания модуля

#define EGTS_PC_MODULE_PROC_FLT 159  // сбой в работе микроконтроллера модуля

#define EGTS_PC_MODULE_SW_FLT 160    // сбой в работе программы модуля

#define EGTS_PC_MODULE_FW_FLT 161    // сбой в работе внутреннего ПО модуля

#define EGTS_PC_MODULE_IO_FLT 162    // сбой в работе блока ввода/вывода модуля

#define EGTS_PC_MODULE_MEM_FLT 163   // сбой в работе внутренней памяти модуля

#define EGTS_PC_TEST_FAILED 164      // тест не пройден

#define NOT_END(it) ASSERT_NE(it, stg.GetEnd())

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

#define CHECKED_FIELD_ACCESS(field)              \
    UINVARIANT(field, "Field ##field is empty"); \
    return *field

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

#define GEN_WORKERS(n) BOOST_PP_REPEAT(n, GEN_WORKER_IMPL, )

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 25

#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 20

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 35

#define PLATFORM_KEY_FUNC(key_name)                                               \
    const TankerKey& key_name(const eats_shared::ApplicationPlatform& platform) { \
        return IsDCApp(platform) ? kDc##key_name : k##key_name;                   \
    }

#define CATASSERT(expr)                                         \
    do {                                                        \
        if (!(expr)) {                                          \
            LOG_ERROR() << "Assertion '" << #expr << "' failed" \
        }                                                       \
    } while (0)

#define CATASSERT_MSG(expr, msg)                                            \
    do {                                                                    \
        if (!(expr)) {                                                      \
            LOG_ERROR() << "Assertion '" << #expr << "' failed: " << (msg); \
        }                                                                   \
    } while (0)

#define EXTRA_VALUE(name, type)                               \
    template <>                                               \
    std::optional<type> ExtraValues::Get() const {            \
        return Get<type>(name);                               \
    }                                                         \
                                                              \
    template <>                                               \
    void ExtraValues::Set(const type& data) const {           \
        Set<type>(name, data);                                \
    }                                                         \
                                                              \
    template <>                                               \
    void ExtraValues::Delete(ExtraValues::Type<type>) const { \
        DeleteValue(name);                                    \
    }

#define MUST_HAVE_VALUE(field)                                             \
    if (!(item.field).has_value()) {                                       \
        LOG_WARNING() << "place " << item.id << " missing field: " #field; \
        return std::nullopt;                                               \
    }

#define MUST_HAVE_VALUE(field)                                                  \
    do {                                                                        \
        if (!(field).has_value()) {                                             \
            LOG_WARNING() << "place " << place.id << " missing field: " #field; \
            return false;                                                       \
        }                                                                       \
    } while (0)

#define ADD_CHANGE_INFO_C(key) utils::AddChangeInfo(builder, data.key, #key)

#define ADD_CHANGE_INFO_U(key) utils::AddChangeInfo(builder, old_country.key, new_data.key, #key)

#define ADD_CHANGE_INFO_C(key) utils::AddChangeInfo(builder, region.key, #key)

#define ADD_CHANGE_INFO_U(key) utils::AddChangeInfo(builder, old_region.key, new_region.key, #key)

#define ADD_CHANGE_INFO_UO(key) utils::AddChangeInfo(builder, old_region.key, std::make_optional(new_region.key), #key)

#define EXTRACT_3(opt, m1, m2) extract_2(extract_2(opt, m1), m2)

#define EXTRACT_4(opt, m1, m2, m3) extract_2(extract_3(opt, m1, m2), m3)

#define EXTRACT_5(opt, m1, m2, m3, m4) extract_2(extract_4(opt, m1, m2, m3), m4)

#define EXTRACT_6(opt, m1, m2, m3, m4, m5) extract_2(extract_5(opt, m1, m2, m3, m4), m5)

#define EXTRACT_7(opt, m1, m2, m3, m4, m5, m6) extract_2(extract_6(opt, m1, m2, m3, m4, m5), m6)

#define EXTRACT_MACRO_CHOOSER(...) \
    GET_8TH_ARG(__VA_ARGS__, extract_7, extract_6, extract_5, extract_4, extract_3, extract_2)

#define EXTRACT(...) EXTRACT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                            \
    namespace storages::postgres::io {                                                               \
    namespace traits {                                                                               \
    template <>                                                                                      \
    struct Input<codegen_type> {                                                                     \
        using Converter = eats_emergency_communications::utils::json::JsonPgConverter<codegen_type>; \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;       \
    };                                                                                               \
    template <>                                                                                      \
    struct Output<codegen_type> {                                                                    \
        using Converter = eats_emergency_communications::utils::json::JsonPgConverter<codegen_type>; \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                               \
    }                                                                                                \
    template <>                                                                                      \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                   \
    }

#define REDIS_KEY_PART(x) #x, (x)

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_layout_configurator::utils::json::JsonPgConverter<codegen_type>;   \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_layout_configurator::utils::json::JsonPgConverter<codegen_type>;   \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define T_LC_LOCALIZE_SIGNATURE                                                          \
    asig::StringType, asig::Optional<asig::IntType>, asig::Optional<asig::ObjectType<>>, \
        asig::Optional<asig::BooleanType>, asig::Optional<asig::StringType>, asig::Optional<asig::StringType>

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_layout_constructor::utils::json::JsonPgConverter<codegen_type>;    \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_layout_constructor::utils::json::JsonPgConverter<codegen_type>;    \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_menu_tags::utils::JsonPgConverter<codegen_type>;                   \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_menu_tags::utils::JsonPgConverter<codegen_type>;                   \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

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

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_order_state::helpers::JsonPgConverter<codegen_type>;               \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_order_state::helpers::JsonPgConverter<codegen_type>;               \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 15

#define COMPARE_AND_UPDATE_FIELD(service_name, mode, new_value, old_value) \
    eats_orders_info::compare_utils::CompareAndUpdateField(#old_value, service_name, mode, new_value, old_value);

#define UPDATE_KWARG_FROM_REQUEST(KWARGS, CONTEXT, METHOD, REQUEST) \
    {                                                               \
        if (REQUEST.CONTEXT.METHOD().has_value()) {                 \
            KWARGS.Update##METHOD(REQUEST.CONTEXT.METHOD##Value()); \
        }                                                           \
    }

#define UPDATE_KWARG(KWARGS, METHOD, VALUE) \
    {                                       \
        KWARGS.Update##METHOD(VALUE);       \
    }

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

#define FILL_SEARCH_PARAM(REQ_FIELD, PARAM_FIELD, TYPE)                  \
    if (request.body.REQ_FIELD.has_value()) {                            \
        params.PARAM_FIELD.insert(TYPE(request.body.REQ_FIELD.value())); \
    }

#define UPDATE_PARTNER_NEW(field) \
    if (request.body.field.has_value()) partner_new.field = request.body.field

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

#define TO_CONSTRUCTOR_ITEM(return_t, input_t)                                                                    \
    constructor::return_t DetailsTemplateParser::ToConstructorItem(const details_template::input_t& item) const { \
        try {                                                                                                     \
            auto ctor_item = constructor::Parse(item.extra, formats::parse::To<constructor::return_t>{});         \
            return ctor_item;                                                                                     \
        } catch (const std::exception& err) {                                                                     \
            throw DetailsTemplateException(err, #return_t);                                                       \
        }                                                                                                         \
    }

#define TO_CONSTRUCTOR_ITEM_RAW(return_t, input_t)                                                                   \
    constructor::return_t DetailsTemplateParser::ToConstructorItemRaw(const details_template::input_t& item) const { \
        try {                                                                                                        \
            auto ctor_item = constructor::Parse(item.extra, formats::parse::To<constructor::return_t>{});            \
            return ctor_item;                                                                                        \
        } catch (const std::exception& err) {                                                                        \
            throw DetailsTemplateException(err, #return_t);                                                          \
        }                                                                                                            \
    }

#define ADD(field) builder[#field] = field

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_place_collections::utils::json::JsonPgConverter<codegen_type>;     \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_place_collections::utils::json::JsonPgConverter<codegen_type>;     \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define MUST_HAVE_VALUE(field)                                                      \
    do {                                                                            \
        if (!field.has_value()) {                                                   \
            LOG_ERROR() << "Place " << place.place_id << " missing field: " #field; \
            return false;                                                           \
        }                                                                           \
    } while (0)

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_place_leaders::utils::JsonPgConverter<codegen_type>;               \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_place_leaders::utils::JsonPgConverter<codegen_type>;               \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define MUST_HAVE_VALUE(field)                                                     \
    do {                                                                           \
        if (!field.has_value()) {                                                  \
            LOG_ERROR() << "Place " << place.place_id << " has no field: " #field; \
            return false;                                                          \
        }                                                                          \
    } while (0)

#define FILL(KIND_ENUM, FIELD)                       \
    if (component.kind(0) == maps_kind::KIND_ENUM) { \
        result.FIELD = component.name();             \
    }

#define INIT_UPSERTER(UPSERTER_NAME)                                         \
    class UPSERTER_NAME : public IUpserter {                                 \
    public:                                                                  \
        std::string GetMetricLabel() const override;                         \
                                                                             \
        bool Upsert(                                                         \
            storages::postgres::Transaction& trx,                            \
            handlers::libraries::eats_place_info::PlaceLogbrokerData&& place \
        ) const override;                                                    \
    };

#define MUST_HAVE_VALUE(field)                                                  \
    do {                                                                        \
        if (!field.has_value()) {                                               \
            LOG_WARNING() << "place " << place.id << " missing field: " #field; \
            return false;                                                       \
        }                                                                       \
    } while (0)

#define CONTINUOUS_BASIC_TEST Y_CAT(Y_CAT(ContinuousV, EPC_ALGO_VERSION), Test)

#define TEST_VERSION Y_CAT(V, EPC_ALGO_VERSION)

#define CONTINUOUS_EMPTY_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousEmpty)

#define CONTINUOUS_FEES_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousFees)

#define CONTINUOUS_SIMPLIFIED_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ContinuousSimplified)

#define EPC_ALGO_VERSION 1

#define EPC_ALGO_VERSION_NS Y_CAT(v, EPC_ALGO_VERSION)

#define EPC_ALGO_VERSION 15

#define EPC_ALGO_VERSION 16

#define CONTINUOUS_SIMPLIFIED_TEST Y_CAT(Y_CAT(ContinuousV, EPC_ALGO_VERSION), SimplifiedTest)

#define EPC_ALGO_VERSION 17

#define ALGO_LOG(level, cfg_level)                                                                    \
    for (bool _algo_log_fire = ::algorithms::global::ShouldLog((level), (cfg_level)); _algo_log_fire; \
         _algo_log_fire = false)                                                                      \
    LOG(level)

#define THRESHOLDS_BASIC_TEST Y_CAT(Y_CAT(ThresholdsV, EPC_ALGO_VERSION), Test)

#define THRESHOLDS_EMPTY_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsEmpty)

#define THRESHOLDS_FEES_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsFees)

#define THRESHOLDS_SIMPLIFIED_TEST Y_CAT(Y_CAT(TestV, EPC_ALGO_VERSION), ThresholdsSimplified)

#define EPC_ALGO_VERSION 21

#define EPC_ALGO_VERSION 22

#define THRESHOLDS_SIMPLIFIED_TEST Y_CAT(Y_CAT(ThresholdsV, EPC_ALGO_VERSION), SimplifiedTest)

#define EPC_ALGO_VERSION 23

#define EPC_ALGO_VERSION 24

#define INFRA_LOG(logger, level) \
    for (bool _infra_log_fire = (logger).ShouldLog(level); _infra_log_fire; _infra_log_fire = false) LOG(level)

#define CHECK_POSITIVE(PROPERTY)                                                                          \
    if (data.PROPERTY.value_or(0) < 0) {                                                                  \
        LOG_LIMITED_WARNING()                                                                             \
            << log_extra << fmt::format("Negative value: {} = {}", #PROPERTY, data.PROPERTY.value_or(0)); \
        return false;                                                                                     \
    }

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

#define ADD_SUFFIX(suffix, ...) __VA_ARGS__##suffix

#define WITH_END(seq) ADD_SUFFIX(_END, seq)

#define RECURSIVE(macro, seq) WITH_END(macro seq)

#define DECLARE_ATTRIBUTE(x, y) \
    x y{};                      \
    DECLARE_ATTRIBUTE_ODD

#define DECLARE_ATTRIBUTE_ODD(x, y) \
    x y{};                          \
    DECLARE_ATTRIBUTE_EVEN

#define DECLARE_ATTRIBUTE_EVEN(x, y) \
    x y{};                           \
    DECLARE_ATTRIBUTE_ODD

#define DECLARE_ATTRIBUTE_ODD_END

#define DECLARE_ATTRIBUTE_EVEN_END

#define DECLARE_ATTRIBUTES(attributes) RECURSIVE(DECLARE_ATTRIBUTE, attributes)

#define ATTRIBUTE_NAME(_, y) #y ATTRIBUTE_NAME_ODD

#define ATTRIBUTE_NAME_ODD(_, y) , #y ATTRIBUTE_NAME_EVEN

#define ATTRIBUTE_NAME_EVEN(_, y) , #y ATTRIBUTE_NAME_ODD

#define ATTRIBUTE_NAME_ODD_END

#define ATTRIBUTE_NAME_EVEN_END

#define ATTRIBUTES_NAMES(attributes) RECURSIVE(ATTRIBUTE_NAME, attributes)

#define DEFINE_STRUCT(name, attributes)                                                           \
    struct name {                                                                                 \
        static std::vector<std::string> _attributes_names() {                                     \
            static const std::vector<std::string> attributes_names{ATTRIBUTES_NAMES(attributes)}; \
            return attributes_names;                                                              \
        }                                                                                         \
        DECLARE_ATTRIBUTES(attributes)                                                            \
    };

#define DECLARE_FILL_FUNC(field) \
    [this](const eats_report_storage::types::sync::Value& value) { field = value.As<decltype(field)>(); }

#define ATTRIBUTE_FUNC(_, y) {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_ODD

#define ATTRIBUTE_FUNC_ODD(_, y) , {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_EVEN

#define ATTRIBUTE_FUNC_EVEN(_, y) , {#y, DECLARE_FILL_FUNC(y)} ATTRIBUTE_FUNC_ODD

#define ATTRIBUTE_FUNC_ODD_END

#define ATTRIBUTE_FUNC_EVEN_END

#define ATTRIBUTES_TO_FUNC_MAP(attributes) RECURSIVE(ATTRIBUTE_FUNC, attributes)

#define ATTRIBUTE_DEFINITION(_, y) y ATTRIBUTE_DEFINITION_ODD

#define ATTRIBUTE_DEFINITION_ODD(_, y) , y ATTRIBUTE_DEFINITION_EVEN

#define ATTRIBUTE_DEFINITION_EVEN(_, y) , y ATTRIBUTE_DEFINITION_ODD

#define ATTRIBUTE_DEFINITION_ODD_END

#define ATTRIBUTE_DEFINITION_EVEN_END

#define ATTRIBUTES_LIST(attributes) RECURSIVE(ATTRIBUTE_DEFINITION, attributes)

#define DEFINE_PARSABLE_STRUCT(name, attributes)                                                                    \
    struct name {                                                                                                   \
    private:                                                                                                        \
        const std::unordered_map<std::string, eats_report_storage::utils::FieldFillInterface> fill_ops_;            \
                                                                                                                    \
    public:                                                                                                         \
        DECLARE_ATTRIBUTES(attributes)                                                                              \
        const std::unordered_map<std::string, eats_report_storage::utils::FieldFillInterface>& GetFillOps() const { \
            return fill_ops_;                                                                                       \
        }                                                                                                           \
        auto Introspect() { return std::tie(ATTRIBUTES_LIST(attributes)); }                                         \
        name()                                                                                                      \
            : fill_ops_({ATTRIBUTES_TO_FUNC_MAP(attributes)})                                                       \
        {}                                                                                                          \
    };                                                                                                              \
                                                                                                                    \
    inline name Parse(eats_report_storage::types::sync::Row&& row, ::formats::parse::To<name>) {                    \
        name result;                                                                                                \
                                                                                                                    \
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

#define CHECK_POSITIVE(PROPERTY)                 \
    data.PROPERTY.value() *= -1;                 \
    ASSERT_EQ(sync::IsValidYTData(data), false); \
    data.PROPERTY.value() *= -1;

#define CHECK_MAX_VALUES(PROPERTY, PROPERTY_DELTA, MAX_VALUE) \
    old_delta_value = data.PROPERTY_DELTA.value();            \
    old_property_value = data.PROPERTY.value();               \
    data.PROPERTY_DELTA.value() = -1;                         \
    data.PROPERTY.value() = MAX_VALUE;                        \
    ASSERT_EQ(sync::IsValidYTData(data), false);              \
    data.PROPERTY_DELTA.value() = old_delta_value;            \
    data.PROPERTY.value() = old_property_value;

#define CHECK_DELTAS(PROPERTY, PROPERTY_DELTA)               \
    old_value = data.PROPERTY_DELTA.value();                 \
    data.PROPERTY_DELTA.value() = data.PROPERTY.value() + 1; \
    ASSERT_EQ(sync::IsValidYTData(data), false);             \
    data.PROPERTY_DELTA.value() = old_value;

#define CHECK_TIME_PROPERTIES(PROPERTY)          \
    old_value = data.PROPERTY.value();           \
    data.PROPERTY.value() = 3000;                \
    ASSERT_EQ(sync::IsValidYTData(data), false); \
    data.PROPERTY.value() = old_value;

#define MUST_HAVE_VALUE(field)                                              \
    if (!field.has_value()) {                                               \
        LOG_WARNING() << "Place " << place.id << " missing field: " #field; \
        return false;                                                       \
    }

#define MUST_HAVE_VALUE(field)                                              \
    if (!place.field.has_value()) {                                         \
        LOG_WARNING() << "place " << place.id << " missing field: " #field; \
        return false;                                                       \
    }

#define MUST_HAVE_VALUE(field)                                                \
    do {                                                                      \
        if (!field.has_value()) {                                             \
            LOG_ERROR() << "Place " << place.id << " missing field: " #field; \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define CHECK_INFO(field)                                                 \
    if (!field.has_value()) {                                             \
        LOG_ERROR() << "Eater " << eater_id << " missing field: " #field; \
        return std::nullopt;                                              \
    }

#define CHECK_INFO(field)                                                          \
    if (!field.has_value()) {                                                      \
        LOG_ERROR() << "Eater of order " << order_nr << " missing field: " #field; \
        return std::nullopt;                                                       \
    }

#define DELIVERY_ZONE_FEATURE(ID, NAME, ENABLED)                                                      \
    handlers::DeliveryZoneFeatureV3 {                                                                 \
        .type = handlers::PropertyTypeFeature::kFeature, .properties = handlers::DeliveryZoneInfoV3 { \
            .id = ID, .name = NAME, .enabled = ENABLED,                                               \
        }                                                                                             \
    }

#define MUST_HAVE_VALUE(field)                                                       \
    do {                                                                             \
        if (!field.has_value()) {                                                    \
            LOG_ERROR() << fmt::format("Place {} missing field: " #field, place.id); \
            return false;                                                            \
        }                                                                            \
    } while (0)

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = eats_seo::utils::JsonPgConverter<codegen_type>;                         \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = eats_seo::utils::JsonPgConverter<codegen_type>;                         \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define UNIMPLEMENTED_METHOD(ReturnType, MethodName, RequestType)     \
    ReturnType MethodName(                                            \
        const RequestType& /*request*/,                               \
        const clients::eats_eaters::CommandControl& /*cc*/            \
    ) const override {                                                \
        throw std::logic_error("not implemented");                    \
    }                                                                 \
    ::clients::codegen::ResponseFuture<ReturnType> Async##MethodName( \
        const RequestType& /*request*/,                               \
        const clients::eats_eaters::CommandControl& /*cc*/            \
    ) const override {                                                \
        throw std::logic_error("not implemented");                    \
    }

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
    XX(ADVERTISER_COUNTRY_AND_MARKETPLACE_REGION,                                                    \
       AdvertiserCountryAndMarketplaceRegion,                                                        \
       AdvertiserCountryAndMarketplaceRegion,                                                        \
       AdvertiserCountryAndMarketplaceRegion)                                                        \
    XX(COST_GREATER_THAN_BID, CostGreaterThanBid, CostGreaterThanBid, CostGreaterThanBid)

#define ENUM_FIELD(EnumName, Name, Market, Lavka) EnumName,

#define NAME_FIELD(EnumName, Name, Market, Lavka) #Name,

#define LAVKA_FIELD_FUNCTION_PAIR(EnumName, Name, Market, Lavka) {LavkaValidate##Lavka},

#define MARKET_FIELD_FUNCTION_PAIR(EnumName, Name, Market, Lavka) {MarketValidate##Market},

#define COUNT_INVALID_FIELD(Field)                              \
    if (proto.Get##Field() == decltype(proto.Get##Field()){}) { \
        ++invalidCount;                                         \
    }

#define LAVKA_EVENT_FIELD_VALIDATE_FUNC(Field)                                                                        \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) {       \
        Y_UNUSED(index);                                                                                              \
        size_t invalidCount = 0;                                                                                      \
        const ::NMarket::NEventMaster::NApi::TLavkaEventFields& proto = NUtils::NLavka::GetLavkaEventFields(request); \
        COUNT_INVALID_FIELD(Field);                                                                                   \
        return invalidCount;                                                                                          \
    }

#define LAVKA_DOCLOG_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const NMarket::NEventMaster::NApi::TLavkaDocLogFields&                                                  \
            proto = NUtils::NLavka::GetLavkaDocLogFields(request, index);                                       \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

#define LAVKA_RANGER_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const NMarket::NEventMaster::NApi::TRangerFields& proto = request.RangerData.GetRangerFields(index);    \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

#define LAVKA_ALWAYS_VALID_FIELD(Field)                                                                         \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                      \
        Y_UNUSED(index);                                                                                        \
        return 0;                                                                                               \
    }

#define LAVKA_ALWAYS_INVALID_FIELD(Field)                                                                       \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                      \
        Y_UNUSED(index);                                                                                        \
        return 1;                                                                                               \
    }

#define LAVKA_DOCFACTORS_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t LavkaValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                \
        const ::NRanger::DocFactors& proto = request.UrlsRequest.GetDocs(index).GetFactors();                   \
        COUNT_INVALID_FIELD(Field);                                                                             \
        return invalidCount;                                                                                    \
    }

#define MARKET_EVENT_FIELD_VALIDATE_ID_FUNC(Field)                                                               \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(index);                                                                                         \
        const TString value = request.UrlsRequest.GetEventFields().GetMarketEventFields().Get##Field();          \
        const bool invalid = value.empty() || value == "undefined" || value == "-";                              \
        return static_cast<size_t>(invalid);                                                                     \
    }

#define MARKET_EVENT_FIELD_VALIDATE_FUNC(Field)                                                                  \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(index);                                                                                         \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMarketEventFields&                                                   \
            proto = request.UrlsRequest.GetEventFields().GetMarketEventFields();                                 \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define MARKET_IMPRESSION_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TInputDoc& doc = NUtils::NMarket::GetInputDoc(request, index);        \
        const NMarket::NEventMaster::NApi::TMarketImpressionFields&                                              \
            proto = doc.GetDocLogFields().GetMarketDocLogFields().GetImpressionFields();                         \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define MARKET_DOCLOG_FIELD_VALIDATE_FUNC(Field)                                                                       \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) {       \
        size_t invalidCount = 0;                                                                                       \
        const NMarket::NEventMaster::NApi::TInputDoc& doc = NUtils::NMarket::GetInputDoc(request, index);              \
        const NMarket::NEventMaster::NApi::TMarketDocLogFields& proto = doc.GetDocLogFields().GetMarketDocLogFields(); \
        COUNT_INVALID_FIELD(Field);                                                                                    \
        return invalidCount;                                                                                           \
    }

#define MARKET_MADV_LOGFIELD_VALIDATE_FUNC(Field)                                                                \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMadvCommonLogFields&                                                 \
            proto = NUtils::NMarket::GetMadvCommonLogFields(request, index);                                     \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define MARKET_SALE_OFFER_VIEW_ZERO_FEES_SUM_FIELD_VALIDATE_FUNC(Field)                                          \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        const NMarket::NEventMaster::NApi::TMarketSaleOfferView&                                                 \
            proto = NUtils::NMarket::GetMarketSaleOfferView(request, index);                                     \
        const TString& feesString = static_cast<TString>(proto.Get##Field());                                    \
        return ValidateFeesString(feesString);                                                                   \
    }

#define MARKET_SALE_OFFER_VIEW_FIELD_VALIDATE_FUNC(Field)                                                        \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TMarketSaleOfferView&                                                 \
            proto = NUtils::NMarket::GetMarketSaleOfferView(request, index);                                     \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define MARKET_RANGER_FIELD_VALIDATE_FUNC(Field)                                                                 \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const NMarket::NEventMaster::NApi::TRangerFields& proto = request.RangerData.GetRangerFields(index);     \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define MARKET_ALWAYS_VALID_FIELD(Field)                                                                         \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                       \
        Y_UNUSED(index);                                                                                         \
        return 0;                                                                                                \
    }

#define MARKET_ALWAYS_INVALID_FIELD(Field)                                                                       \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        Y_UNUSED(request);                                                                                       \
        Y_UNUSED(index);                                                                                         \
        return 1;                                                                                                \
    }

#define MARKET_DOCFACTORS_FIELD_VALIDATE_FUNC(Field)                                                             \
    size_t MarketValidate##Field(const market_urls_infra::TRequestWithRangerData& request, const size_t index) { \
        size_t invalidCount = 0;                                                                                 \
        const ::NRanger::DocFactors& proto = NUtils::NMarket::GetInputDoc(request, index).GetFactors();          \
        COUNT_INVALID_FIELD(Field);                                                                              \
        return invalidCount;                                                                                     \
    }

#define REGISTER_DETECTOR(TYPE)                                                  \
    namespace {                                                                  \
    struct TYPE##Registrar {                                                     \
        TYPE##Registrar() {                                                      \
            anomalies::detectors::DetectorsRegistry::Instance().Register(        \
                TYPE::kType,                                                     \
                [](models::enums::DetectorType det_type,                         \
                   const std::string& anomaly_code,                              \
                   const anomalies::detectors::DetectionDeps& deps) {            \
                    return std::make_unique<TYPE>(det_type, anomaly_code, deps); \
                }                                                                \
            );                                                                   \
        }                                                                        \
    };                                                                           \
    static TYPE##Registrar global_##TYPE##_registrar;                            \
    }  // namespace

#define LOG_CUSTOM_DEBUG() LOG(utils::logger::GetCtx().logging_level)

#define GET_VALUE_OR_THROW(obj, field) GetValueOrThrow(obj, &std::decay_t<decltype(obj)>::field, #field)

#define READ_VALUE_TOKEN_TO_VAR_AND_MOVE(token_name)                                           \
    std::string_view token_name##_token = lexer.next_token();                                  \
    if (token_name##_token == "," || token_name##_token == ";" || token_name##_token == "=" || \
        token_name##_token.empty())                                                            \
    {                                                                                          \
        return std::nullopt;                                                                   \
    }                                                                                          \
    lexer.move_forward();

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

#define CONVERT_TOKEN_IF_NOT_NULL(token_name, to_type, assign_to_var, assign_expr) \
    {                                                                              \
        if (token_name##_token != "null") {                                        \
            CONVERT_TOKEN(token_name, to_type, assign_to_var, assign_expr);        \
        }                                                                          \
    }

#define NEW_SUBITEMS_CALC_LOG(X) LOG_INFO() << "NEW_SUBITEMS_CALC " #X ": [" << X << "]"

#define LOG_PRICING(X) LOG_INFO() << "[PRICING] " #X ": [" << X << "]"

#define LOG_INFO_OPT(enabled, ...) LOG(enabled ? logging::Level::kInfo : logging::Level::kNone, __VA_ARGS__)

#define LOG_WARNING_OPT(enabled, ...) LOG(enabled ? logging::Level::kWarning : logging::Level::kNone, __VA_ARGS__)

#define CHECK_NOT_NULLOPT(x) CheckNotNullopt((x), #x)

#define CHECK_FIELD(field)                                                                            \
    do {                                                                                              \
        if (!item.field.has_value()) {                                                                \
            LOG_DEBUG("Zone {} is missing field '{}'. Will not add zone to cache.", item.id, #field); \
            return std::nullopt;                                                                      \
        }                                                                                             \
    } while (0)

#define GET_PARAM(param_name) ((override && override->param_name) ? *override->param_name : settings.param_name)

#define INITIALIZE(param_name) .param_name = GET_PARAM(param_name)

#define STATISTICS_IMPL

#define CARING_BASIC_TYPES_HPP_SILENCE_DEPRECATION

#define CLIENTS_DATUM_IMPL

#define STAGES_ENUM_MAP(XX)                   \
    XX(CalculatingOffer, "calculating_offer") \
    XX(Ordercommit, "ordercommit")

#define ERROR_REASON_ENUM_MAP(XX)                    \
    XX(GetCorpTariffFail, "get_corp_tarif_fail")     \
    XX(PluginInternalError, "plugin_internal_error") \
    XX(NoTariffPrice, "no_tariff_price")             \
    XX(UnknownError, "unknown_error")

#define PLUGIN_RETHROW(exc) throw common::plugins::RethrowableException(std::make_exception_ptr(exc))

#define UNAVAILABLE_REASONS_ENUM_MAP(XX)                                           \
    XX(PreorderUnavailableForDue, "preorder_unavailable_for_due")                  \
    XX(PreorderUnavailableForPaymentType, "preorder_unavailable_for_payment_type") \
    XX(PreorderUnavailableForTariff, "preorder_unavailable_for_tariff")            \
    XX(PreorderUnavailableForRequirements, "preorder_unavailable_for_requirements")

#define BILLING_CLEANUP_CONTRACT_ON_AUTOREORDER_ENUM_MAP(XX) \
    XX(kDisabled, "disabled")                                \
    XX(kJustLogging, "just_logging")                         \
    XX(kEnabled, "enabled")

#define INIT_MEMBER(name_) name_(boost::to_upper_copy(std::string("MULTICLASS_" #name_)), docs_map)

#define INIT_MEMBER(name_) name_(boost::to_upper_copy(std::string("MULTIORDER_" #name_)), docs_map)

#define ESTIMATE_COST(XX) XX(MINIMAL_COST, "MINIMAL_COST")

#define ESTIMATE_ENUM_MAP(XX)                        \
    XX(CANT_CONSTRUCT_ROUTE, "CANT_CONSTRUCT_ROUTE") \
    XX(CURFEW, "CURFEW")

#define HOOK_ENUM_MAP(XX)                \
    XX(BaseCalcStart, "base_calc_start") \
    XX(BaseCalcEnd, "base_calc_end")

#define HOOK_ENUM_MAP(XX) XX(PendingEnd, "pending_end")

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
    XX(DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS,                                                  \
       "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS")                                                \
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

#define ORDERDRAFT_CHECK_LIFETIME_POINT_ENUM_MAP(XX) \
    XX(CommitInit, "commit_init")                    \
    XX(CommitPendingStart, "commit_pending_start")   \
    XX(CommitPendingFinish, "commit_pending_finish")

#define OPPORTUNITIES_ENUM_MAP(XX) \
    XX(Allowed, "allowed")         \
    XX(Disallowed, "disallowed")   \
    XX(NotModified, "not_modified")

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 11

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 12

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 16

#define STAGES_ENUM_MAP(XX)                    \
    XX(kCalculatingOffer, "calculating_offer") \
    XX(kOrdercommit, "ordercommit")

#define ERROR_REASON_ENUM_MAP(XX)                     \
    XX(kGetCorpTariffFail, "get_corp_tarif_fail")     \
    XX(kPluginInternalError, "plugin_internal_error") \
    XX(kNoTariffPrice, "no_tariff_price")             \
    XX(kUnknownError, "unknown_error")

#define UNAVAILABLE_REASONS_ENUM_MAP(XX)                                            \
    XX(kPreorderUnavailableForDue, "preorder_unavailable_for_due")                  \
    XX(kPreorderUnavailableForPaymentType, "preorder_unavailable_for_payment_type") \
    XX(kPreorderUnavailableForTariff, "preorder_unavailable_for_tariff")            \
    XX(kPreorderUnavailableForRequirements, "preorder_unavailable_for_requirements")

#define ASSERT_CLOSE_BBOXES(first, second) \
    ASSERT_TRUE(geometry::AreCloseBoundingBoxes(first, second)) << "first: " << (first) << "\nsecond: " << (second);

#define MARKET_ADVERT_SEARCH_INCUTS_CORE_ERROR_LOG_IMPL_METRICS_METRICS_INL_HPP

#define ECPM_LOG_ERROR_TO(logger, code) logger.CreateEntry(code)

#define EXPECT_EQ_PROTO(lhs, rhs) EXPECT_PRED2(::google::protobuf::util::MessageDifferencer::Equals, lhs, rhs);

#define ECPM_LOG_DEBUG() LOG_DEBUG()

#define ECPM_LOG_INFO() LOG_INFO()

#define ECPM_LOG_WARN() LOG_WARNING()

#define ECPM_LOG_ERROR(code) ECPM_LOG_ERROR_TO(market_advert_search_incuts::core::error_log::GetErrorLogger(), code)

#define LOG_AND_INCREMENT_WARNING() \
    ++response.warnings_count;      \
    LOG_WARNING()

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

#define GET_REQUEST(field, build_key) GET_REQUEST_WITH_METRIC(field, build_key, field)

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

#define PROTO_TO_PTR_OR(obj, field, else_value) (obj.Has##field() ? &obj.Get##field() : else_value)

#define PROTO_TO_PTR(obj, field) PROTO_TO_PTR_OR(obj, field, nullptr)

#define PROTO_TO_MUT_PTR(obj, field) (obj.Has##field() ? obj.Mutable##field() : nullptr)

#define COPY_RANGE_TO_FILTER_REPEATED(request_field, filter_field) \
    filter.mutable_##filter_field()->Assign(request.request_field.begin(), request.request_field.end())

#define COPY_VECTOR_TO_FILTER_JOINED(request_field, filter_field)             \
    if (!request.request_field.empty()) {                                     \
        filter.set_##filter_field(JoinCommaSeparated(request.request_field)); \
    }

#define COPY_OPTIONAL_TO_FILTER(request_field, filter_field) \
    if (request.request_field) {                             \
        filter.set_##filter_field(*request.request_field);   \
    }

#define COPY_OPTIONAL_TO_FILTER_STRINGIFIED(request_field, filter_field)       \
    if (request.request_field) {                                               \
        filter.set_##filter_field(TStringBuilder() << *request.request_field); \
    }

#define COPY_OPTIONAL_BOOL_TO_FILTER_01(request_field, filter_field)   \
    if (request.request_field) {                                       \
        filter.set_##filter_field(*request.request_field ? "1" : "0"); \
    }

#define COPY_OPTIONAL_COMMA_TO_FILTER_REPEATED(request_field, filter_field)             \
    if (request.request_field) {                                                        \
        AssignCommaSeparated(*filter.mutable_##filter_field(), *request.request_field); \
    }

#define COPY_FILTER_REPEATED_TO_REQUEST_VECTOR(filter_field, request_field) \
    request.request_field.assign(filter.filter_field().begin(), filter.filter_field().end())

#define COPY_FILTER_JOINED_TO_REQUEST_VECTOR(filter_field, request_field) \
    AssignRepeatedString(filter.has_##filter_field(), filter.filter_field(), request.request_field)

#define COPY_FILTER_TO_REQUEST_OPTIONAL(filter_field, request_field) \
    AssignOptionalString(filter.has_##filter_field(), filter.filter_field(), request.request_field)

#define COPY_FILTER_TO_REQUEST_INT(filter_field, request_field)                                                 \
    if (!AssignInt(filter.has_##filter_field(), filter.filter_field(), request.request_field, #filter_field)) { \
        return;                                                                                                 \
    }

#define COPY_FILTER_TO_REQUEST_BOOL(filter_field, request_field)                                                 \
    if (!AssignBool(filter.has_##filter_field(), filter.filter_field(), request.request_field, #filter_field)) { \
        return;                                                                                                  \
    }

#define COPY_FILTER_REPEATED_JOINED_TO_REQUEST_OPTIONAL(filter_field, request_field) \
    AssignJoinedString(filter.filter_field(), request.request_field)

#define TRACE_BLENDER_DEBUG(where, what, ...) TRACE_ME(::NMarketBlender::GetBlenderTrace(where, what, __VA_ARGS__))

#define ASSIGN_IF_HAS_VALUE(field, proto_message)                                  \
    if (proto_message.has_##field()) {                                             \
        if constexpr (std::is_floating_point_v<decltype(proto_message.field())>) { \
            FillFloating(result.field, proto_message.field());                     \
        } else {                                                                   \
            result.field = proto_message.field();                                  \
        }                                                                          \
    }

#define FILL_IF_HAS_VALUE(field, proto_message, fill_func)                 \
    if (proto_message.has_##field()) {                                     \
        fill_func(proto_message.field(), EmplaceIfOptional(result.field)); \
    }

#define FILL_ARRAY_IF_NOT_EMPTY(field, proto_message, fill_func)   \
    if (int size = proto_message.field##_size()) {                 \
        auto& result_field = EmplaceIfOptional(result.field);      \
        result_field.reserve(size);                                \
        for (const auto& proto_element : proto_message.field()) {  \
            fill_func(proto_element, result_field.emplace_back()); \
        }                                                          \
    }

#define COPY_ARRAY_IF_NOT_EMPTY(field, proto_message) FILL_ARRAY_IF_NOT_EMPTY(field, proto_message, DoCopyAssign)

#define INSTANTIATE(Type) \
    template void TSnippetsMap::CreateSnippetServiceProxyData(std::string_view, std::string_view, const Type&, std::optional<handlers::SnippetServiceProxyDataObject>&, std::optional<handlers::SnippetServiceProxyDataString>&);

#define NAMED_CGI_PARAM(NAME, VALUE) fmt::format("{}={}", NAME, VALUE)

#define CGI_PARAM(X) NAMED_CGI_PARAM(#X, X)

#define FILL_VALUE(result, proto_message, field)   \
    if (proto_message.has_##field()) {             \
        result.field = proto_message.Get##field(); \
    }

#define LOG_BOLAT_DEBUG() LOG(market_bolat::logger::GetCtx().logging_level)

#define Y_PARAM_GET(type, param, name) param##_ = Initialize<type>(factors_map, #name);

#define Y_PARAM_OPT(type, param, name) param##_ = Initialize<type>(factors_map, #name);

#define Y_PARAM_TRY(type, param, name) param##_ = Initialize<type>(factors_map, #name);

#define Y_PARAM_DEF(type, param, name, def) param##_ = Initialize<type>(factors_map, #name);

#define Y_MESSAGE_DEF(type, message, param, name, def) message##param##_ = Initialize<type>(factors_map, #name);

#define Y_VECTOR_DEF(type, param, name, def) param##_ = Initialize<type>(factors_map, #name);

#define Y_PARAM_GET(type, param, name)                                                                      \
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

#define Y_PARAM_TRY(type, param, name)                                                                \
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

#define Y_PARAM_DEF(type, param, name, def)                                          \
public:                                                                              \
    bool Has##param() const final { return true; }                                   \
    type Get##param() const final { return param##_.has_value() ? *param##_ : def; } \
                                                                                     \
private:                                                                             \
    std::expected<type, InitializationError> param##_;

#define Y_VECTOR_DEF(type, param, name, def)                                            \
public:                                                                                 \
    bool Has##param() const final { return true; }                                      \
    type Get##param() const final {                                                     \
        if (param##_.has_value()) {                                                     \
            return *param##_;                                                           \
        } else {                                                                        \
            /* Может инициализировать std::vector и std::array */ \
            if constexpr (std::is_array_v<std::remove_reference_t<decltype(def)>>) {    \
                return type(std::begin(def), std::end(def));                            \
            } else {                                                                    \
                return type(def.begin(), def.end());                                    \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
                                                                                        \
private:                                                                                \
    std::expected<type, InitializationError> param##_;

#define Y_MESSAGE_DEF(type, message, param, name, def)                                                          \
public:                                                                                                         \
    bool Has##message##param() const final { return true; }                                                     \
    type Get##message##param() const final { return message##param##_.has_value() ? *message##param##_ : def; } \
                                                                                                                \
private:                                                                                                        \
    std::expected<type, InitializationError> message##param##_;

#define Y_PARAM_OPT(type, param, name)                                                                \
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

#define GET_OPTIONAL(src, field) src.has_##field() ? std::make_optional(src.field()) : std::nullopt

#define DECLARE_LOG_HELPERS(type)            \
    std::string ToString(const type& value); \
                                             \
    ::logging::LogHelper& operator<<(::logging::LogHelper& lh, const type& value);

#define DEFINE_LOG_HELPERS(type)              \
    std::string ToString(const type& value) { \
        formats::json::StringBuilder builder; \
        WriteToStream(value, builder);        \
        return builder.GetString();           \
    }                                         \
                                              \
    ::logging::LogHelper& operator<<(::logging::LogHelper& lh, const type& value) { return lh << ToString(value); }

#define LOG_SLOWPOKE_DEBUG() LOG(market_category_storage::util::GetLoggerContext().logging_level)

#define LOG_CS_DEBUG() LOG_DEBUG() << kLogComponentTag << ": "

#define LOG_CS_DEBUG() LOG(market_content_storage_userver::util::GetLoggerContext().logging_level)

#define LOG_CUSTOM_DEBUG() LOG(market_delivery_actualizer::utils::logger::GetCtx().logging_level)

#define MDA_CREATE_METRIC(p_module, p_variable, p_tag, p_seq) I_MDA_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)

#define MDA_CREATE_METRIC_STRUCT(p_module, p_seq) \
    I_MDA_GENERATE_BASE_METRIC_STRUCT(p_module, BOOST_PP_CAT(I_MDA_CONVERT_SEQ_X p_seq, 0))

#define MDA_REGISTER_METRIC(p_module, p_variable, p_tag)                 \
    I_MDA_CREATE_INHERITED_METRIC_STRUCT(p_module, p_module##p_variable) \
    I_MDA_REGISTER_METRIC(p_module##p_variable, p_variable, p_tag)

#define I_MDA_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)                           \
    I_MDA_GENERATE_BASE_METRIC_STRUCT(p_module, BOOST_PP_CAT(I_MDA_CONVERT_SEQ_X p_seq, 0)) \
    I_MDA_REGISTER_METRIC(p_module, p_variable, p_tag)

#define I_MDA_CONVERT_SEQ_X(x, y) ((x, y)) I_MDA_CONVERT_SEQ_Y

#define I_MDA_CONVERT_SEQ_Y(x, y) ((x, y)) I_MDA_CONVERT_SEQ_X

#define I_MDA_CONVERT_SEQ_X0

#define I_MDA_CONVERT_SEQ_Y0

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

#define I_MDA_CREATE_INHERITED_METRIC_STRUCT(p_base_module, p_module)                                    \
    struct p_module : public p_base_module {};                                                           \
                                                                                                         \
    [[maybe_unused]] inline void DumpMetric(::utils::statistics::Writer& writer, const p_module& stat) { \
        p_base_module##_DumpMetricImpl(writer, stat);                                                    \
    }                                                                                                    \
                                                                                                         \
    [[maybe_unused]] inline void ResetMetric(p_module& stat) { p_base_module##_ResetMetricImpl(stat); }

#define I_MDA_REGISTER_METRIC(p_module, p_variable, p_tag) \
    inline const ::utils::statistics::MetricTag<p_module> p_variable{mda::metrics::CreateMetricPath(p_tag)};

#define I_MDA_REGISTER_INHERITED_STRUCT(p_base_module, p_module, p_variable, p_tag)

#define I_MDA_METRICS_DEFINE_FIELDS(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_DEFINE_FIELDS_OP, _, p_seq)

#define I_MDA_METRICS_DEFINE_FIELDS_OP(p_r, p_data, p_elem) \
    ::utils::statistics::RateCounter BOOST_PP_TUPLE_ELEM(0, p_elem);

#define I_MDA_METRICS_GENERATE_DUMPING(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_GENERATE_DUMPING_OP, _, p_seq)

#define I_MDA_METRICS_GENERATE_DUMPING_OP(p_r, p_data, p_elem) \
    writer.ValueWithLabels(stat.BOOST_PP_TUPLE_ELEM(0, p_elem), {"type", BOOST_PP_TUPLE_ELEM(1, p_elem)});

#define I_MDA_METRICS_GENERATE_RESETTING(p_seq) BOOST_PP_SEQ_FOR_EACH(I_MDA_METRICS_GENERATE_RESETTING_OP, _, p_seq)

#define I_MDA_METRICS_GENERATE_RESETTING_OP(p_r, p_data, p_seq) \
    stat.BOOST_PP_TUPLE_ELEM(0, p_seq).Store({0}, std::memory_order_seq_cst);

#define DECLARE_RUN()                                 \
    template <configuration::Environment Environment> \
    static void                                       \
    Run(const ClientType::RequestType&, ClientType::ResponseType&, resources::Context&, const handlers::Dependencies&)

#define DECLARE_RUN_SPECIALIZATION(Env) \
    template <>                         \
    static void                         \
    Run<Env>(const ClientType::RequestType&, ClientType::ResponseType&, resources::Context&, const handlers::Dependencies&)

#define FOR_EACH_MOVE(container, iterator_name)                                  \
    for (auto iterator_name = std::make_move_iterator((container).begin()),      \
              iterator_name##__end = std::make_move_iterator((container).end()); \
         iterator_name != iterator_name##__end;                                  \
         ++iterator_name)

#define TRACE_ME LOG(context.rearr_flags.enable_debug_log() ? logging::Level::kInfo : logging::Level::kDebug)

#define TESTPOINT_VAR_COUNTER(variable)                             \
    TESTPOINT(#variable, ([]() {                                    \
                  auto builder = formats::json::ValueBuilder();     \
                  builder.EmplaceNocheck("value", variable.load()); \
                  return builder.ExtractValue();                    \
              })())

#define TESTPOINT_VAR(variable, name)                               \
    TESTPOINT(name, ([]() {                                         \
                  auto builder = formats::json::ValueBuilder();     \
                  builder.EmplaceNocheck("value", variable.load()); \
                  return builder.ExtractValue();                    \
              })())

#define DYN2YT_CREATE_METRIC(p_module, p_variable, p_tag, p_seq) \
    I_DYN2YT_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq)

#define I_DYN2YT_GENERATE_METRIC(p_module, p_variable, p_tag, p_seq) \
    I_DYN2YT_GENERATE_METRIC_SEQ(p_module, p_variable, p_tag, BOOST_PP_CAT(I_DYN2YT_CONVERT_SEQ_X p_seq, 0))

#define I_DYN2YT_CONVERT_SEQ_X(x, y) ((x, y)) I_DYN2YT_CONVERT_SEQ_Y

#define I_DYN2YT_CONVERT_SEQ_Y(x, y) ((x, y)) I_DYN2YT_CONVERT_SEQ_X

#define I_DYN2YT_CONVERT_SEQ_X0

#define I_DYN2YT_CONVERT_SEQ_Y0

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

#define I_DYN2YT_METRICS_DEFINE_FIELDS(p_seq) BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_DEFINE_FIELDS_OP, _, p_seq)

#define I_DYN2YT_METRICS_DEFINE_FIELDS_OP(p_r, p_data, p_elem) \
    ::utils::statistics::RateCounter BOOST_PP_TUPLE_ELEM(0, p_elem);

#define I_DYN2YT_METRICS_GENERATE_DUMPING(p_tag, p_seq) \
    BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_GENERATE_DUMPING_OP, _, p_seq)

#define I_DYN2YT_METRICS_GENERATE_DUMPING_OP(p_r, p_data, p_elem) \
    writer.ValueWithLabels(stat.BOOST_PP_TUPLE_ELEM(0, p_elem), {"type", BOOST_PP_TUPLE_ELEM(1, p_elem)});

#define I_DYN2YT_METRICS_GENERATE_RESETTING(p_seq) \
    BOOST_PP_SEQ_FOR_EACH(I_DYN2YT_METRICS_GENERATE_RESETTING_OP, _, p_seq)

#define I_DYN2YT_METRICS_GENERATE_RESETTING_OP(p_r, p_data, p_seq) \
    stat.BOOST_PP_TUPLE_ELEM(0, p_seq).Store({0}, std::memory_order_seq_cst);

#define TRY_APPLY_GLOBAL_REDIRECT(functor, context, settings)                               \
    do {                                                                                    \
        auto redirect_result = market_link_fixer::global_redirects::TryApplyGlobalRedirect< \
            decltype(functor),                                                              \
            decltype(settings),                                                             \
            decltype(context)>(url, deps, context, functor, settings);                      \
        RETURN_IF_HAS_VALUE(redirect_result);                                               \
    } while (false)

#define PERFORMACE_PARAMS_RAW                                                                                         \
    "utm_source_service", "clid", "src_pof", "icookie", "baobab_event_id", "wprid", "ysclid", "vsclid", "utm_source", \
        "utm_medium", "utm_campaign", "utm_content", "utm_term", "yclid", "ybaip"

#define RETURN_IF_HAS_VALUE(optional) \
    if (optional.has_value()) {       \
        return optional.value();      \
    }

#define TRACE_ME LOG(context.is_debug_log_enabled ? logging::Level::kInfo : logging::Level::kDebug)

#define EXPERIMENTS3_COMMON_INLINE __attribute__((always_inline, pure))

#define EXPERIMENTS3_COMMON_NOINLINE __attribute__((noinline, cold))

#define THROW_NOT_CREATED(resource)                                 \
    do {                                                            \
        if (!(resource)) {                                          \
            throw std::runtime_error("Couldn't create " #resource); \
        }                                                           \
    } while (0)

#define LOG_SNIPPET() LOG(market_snippet_service::logger::GetLoggingLevel())

#define CHECK(expr)          \
    if (auto err = (expr)) { \
        return err;          \
    }

#define NAME_OF(name) market_walter::utils::NameOf(sizeof(typeid(name)), #name)

#define FORMAT_FIELD(field) \
    (field.has_value() ? std::optional<std::string>(fmt::format("{}={}", NAME_OF(field), field.value())) : std::nullopt)

#define FORMAT_ENUM_FIELD(field)                                                                     \
    (field.has_value()                                                                               \
         ? std::optional<std::string>(fmt::format("{}={}", NAME_OF(field), ToString(field.value()))) \
         : std::nullopt)

#define CREATE_MEMBER_CHECKER(member)                                  \
    template <typename T>                                              \
    struct has_##member {                                              \
        template <typename U>                                          \
        static char Check(decltype(&U::member));                       \
        template <typename U>                                          \
        static int Check(...);                                         \
        static const bool value = sizeof(Check<T>(0)) == sizeof(char); \
    };                                                                 \
    template <typename T>                                              \
    constexpr bool has_##member##_v = has_##member<T>::value;

#define VARIANT_VALUE(setter, ...)                                       \
    ([&]() -> ::offer_layout::proto::VariantValue {                      \
        ::offer_layout::proto::VariantValue _offer_layout_variant_value; \
        _offer_layout_variant_value.setter(__VA_ARGS__);                 \
        return _offer_layout_variant_value;                              \
    }())

#define LOG_OFFER_DEBUG() LOG(market_offerinfo::logger::GetCtx().logging_level)

#define REGISTER_COMPONENT(...)                                                         \
    static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_); \
    template <>                                                                         \
    constexpr bool components::kHasValidate<__VA_ARGS__> = true;

#define REGISTER_COMPONENT_NOSCHEMA(...)                                                \
    static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_); \
    template <>                                                                         \
    constexpr bool components::kHasValidate<__VA_ARGS__> = false;

#define DECLARE_COMPONENT(...) static const bike::ComponentRegister<__VA_ARGS__> Y_GENERATE_UNIQUE_ID(component_)(true);

#define CURRENT_PRICES_WORK_MODE_ENUM_MAP(XX) \
    XX(kOldWay, "oldway")                     \
    XX(kNewWay, "newway")                     \
    XX(kDryRun, "dryrun")                     \
    XX(kTryOut, "tryout")

#define INIT_ORDER_429_METRIC(name, reason)                                                                \
    name.reserve(order_too_many_handlers.size());                                                          \
    for (const auto& handler : order_too_many_handlers) {                                                  \
        name.emplace(                                                                                      \
            handler,                                                                                       \
            InitMetric(                                                                                    \
                solomon,                                                                                   \
                solomon::Metric::Labels{{"sensor", "order_429"}, {"reason", reason}, {"handler", handler}} \
            )                                                                                              \
        );                                                                                                 \
    }

#define MODELS_ORDER_PIN_HPP

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
    XX(kDisabledPaymentTypePersonalWalletIfNoCashbackPlus, "DISABLED_PAYMENT_TYPE_PERSONAL_WALLET_IF_NO_CASHBACK_PLUS" \
    )                                                                                                                  \
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

#define ORDER_SERVICE_TYPE_ENUM_MAP(XX) \
    XX(kCargo, "cargo")                 \
    XX(kTaxi, "taxi")

#define ORDERDRAFT_CHECK_LIFETIME_POINT_ENUM_MAP(XX) \
    XX(kCommitInit, "commit_init")                   \
    XX(kCommitPendingStart, "commit_pending_start")  \
    XX(kCommitPendingFinish, "commit_pending_finish")

#define OPPORTUNITIES_ENUM_MAP(XX) \
    XX(kAllowed, "allowed")        \
    XX(kDisallowed, "disallowed")  \
    XX(kNotModified, "not_modified")

#define EXTRA_PHONE_TYPE_ENUM_MAP(XX) \
    XX(kMain, "main")                 \
    XX(kExtra, "extra")

#define COMMIT_ERROR_ENUM_MAP(XX)                              \
    XX(kTooManyConcurrentOrders, "TOO_MANY_CONCURRENT_ORDERS") \
    XX(kOrderDraftExpired, "ORDER_DRAFT_EXPIRED")

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

#define XLOG_DEBUG()                                             \
    LOG_DEBUG()                                                  \
        << __FILE__ << ":" << __LINE__ << "[" << __func__ << "]" \
        << " xlog: "

#define XXH_INLINE_ALL

#define DEFINE_TAG(ClassName, TagName) \
    template <>                        \
    const ClassName::ValueType ClassName::MetricTagHolder::kMetricTag(#TagName);

#define DECLARE_METHOD(name)                                                                                          \
private:                                                                                                              \
    ResponseExtractor<name##Result>::Type name(name##Request&& request, handlers::Dependencies&& dependencies) const; \
                                                                                                                      \
public:                                                                                                               \
    name##Result name(CallContext& context, name##Request&& request) override {                                       \
        using ThisType = typename std::remove_reference_t<decltype(*this)>;                                           \
        return WrapMethod<                                                                                            \
            name##Request,                                                                                            \
            ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request));   \
    }

#define DECLARE_METHOD(name)                                                                                        \
private:                                                                                                            \
    ResponseExtractor<name##Result>::Type name(                                                                     \
        name##Request&& request,                                                                                    \
        handlers::Dependencies&& dependencies,                                                                      \
        const aip_161_field_mask::FieldMask& field_mask,                                                            \
        const qc_gate_common::permissions::Permissions& permissions,                                                \
        ugrpc::server::CallContext& context                                                                         \
    ) const;                                                                                                        \
                                                                                                                    \
public:                                                                                                             \
    name##Result name(CallContext& context, name##Request&& request) override {                                     \
        using ThisType = typename std::remove_reference_t<decltype(*this)>;                                         \
        return WrapMethod<                                                                                          \
            name##Request,                                                                                          \
            ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request)); \
    }

#define DELETE_COPY(name)                  \
    name(name&&) = default;                \
    name(const name&) = delete;            \
                                           \
    name& operator=(name&&) = default;     \
    name& operator=(const name&) = delete; \
                                           \
    virtual ~name() = default;

#define DELETE_COPY_MOVE(name)             \
    name(name&&) = delete;                 \
    name(const name&) = delete;            \
                                           \
    name& operator=(name&&) = delete;      \
    name& operator=(const name&) = delete; \
                                           \
    virtual ~name() = default;

#define DECLARE_METHOD(name)                                                                                        \
private:                                                                                                            \
    ResponseExtractor<name##Result>::Type name(                                                                     \
        name##Request&& request,                                                                                    \
        handlers::Dependencies&& dependencies,                                                                      \
        const aip_161_field_mask::FieldMask& field_mask                                                             \
    ) const;                                                                                                        \
                                                                                                                    \
public:                                                                                                             \
    name##Result name(CallContext& context, name##Request&& request) override {                                     \
        using ThisType = typename std::remove_reference_t<decltype(*this)>;                                         \
        return WrapMethod<                                                                                          \
            name##Request,                                                                                          \
            ResponseExtractor<name##Result>::Type>(&ThisType::name, context, std::forward<name##Request>(request)); \
    }

#define DECLARE_BIDIRECTIONAL_STREAM(name)                                                                            \
private:                                                                                                              \
    ResponseExtractor<name##Result>::Type name(name##Request&& request, handlers::Dependencies&& dependencies) const; \
                                                                                                                      \
public:                                                                                                               \
    name##Result name(CallContext& context, name##ReaderWriter& stream) override {                                    \
        using ThisType = typename std::remove_reference_t<decltype(*this)>;                                           \
        return WrapBidiretionalStream<name##Request, name##Response>(&ThisType::name, context, stream);               \
    }

#define DECLARE_SERVER_STREAM(name)                                                                        \
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

#define SET_PROTO_FIELD(ProtoPtr, Field, OptValue) \
    if (OptValue.has_value()) {                    \
        ProtoPtr->set_##Field(OptValue.value());   \
    }\

#define SCOOTERS_COORDS_DEFINE_STD_HASH_FOR_LABELS(Type) \
    template <>                                          \
    struct std::hash<Type> : metrics::LabelsHash<Type> {}

#define L(x) #x "=" << (x)

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

#define DECLARE_SCOOTERS_OPS_TRAITS_FUNCTION_PTR_BASE                                \
    using is_callable_traits = std::true_type;                                       \
    using Result = ResultType;                                                       \
    using ArgsTuple = std::tuple<Args...>;                                           \
    template <typename ResultTypeToCheck>                                            \
    constexpr static bool IsCorrectResult = std::same_as<ResultTypeToCheck, Result>; \
    template <typename... ArgsToCheck>                                               \
    constexpr static bool IsCorrectArgs = std::same_as<std::tuple<ArgsToCheck...>, ArgsTuple>;

#define DECLARE_SCOOTERS_OPS_TRAITS_MEMBER_BASE     \
    DECLARE_SCOOTERS_OPS_TRAITS_FUNCTION_PTR_BASE   \
    using Object = std::remove_const_t<ObjectType>; \
    template <utils::Object T>                      \
    constexpr static bool IsObject = std::same_as<std::remove_cvref_t<T>, Object>;

#define TO_CHANGED_FIELD_OVERRIDE(new_val, old_val, is_disabling, string_name, field) \
    old_val                                                                           \
        ? ToChangedField(                                                             \
              string_name,                                                            \
              is_disabling,                                                           \
              new_val,                                                                \
              *old_val,                                                               \
              &std::decay_t<decltype(new_val)>::field,                                \
              &std::decay_t<decltype(*(old_val))>::field                              \
          )                                                                           \
        : ChangedField<decltype(new_val.field)>(string_name, new_val.field, kRequireChecking, kWasNotChanged)

#define TO_CHANGED_FIELD(new_val, old_val, is_disabling, field) \
    TO_CHANGED_FIELD_OVERRIDE(new_val, old_val, is_disabling, FieldName(#field), field)

#define TO_CHANGED_FIELD_NESTED_OVERRIDE(new_val, old_val, is_disabling, string_name, inner_field, field) \
    old_val                                                                                               \
        ? ToChangedField(                                                                                 \
              string_name,                                                                                \
              is_disabling,                                                                               \
              new_val.inner_field,                                                                        \
              old_val->inner_field,                                                                       \
              &decltype(new_val.inner_field)::field,                                                      \
              &decltype(old_val->inner_field)::field                                                      \
          )                                                                                               \
        : ChangedField<decltype(new_val.inner_field.field                                                 \
          )>(string_name, new_val.inner_field.field, kRequireChecking, kWasNotChanged)

#define TO_CHANGED_FIELD_NESTED(new_val, old_val, is_disabling, inner_field, field) \
    TO_CHANGED_FIELD_NESTED_OVERRIDE(                                               \
        new_val,                                                                    \
        old_val,                                                                    \
        is_disabling,                                                               \
        FieldName(#inner_field "." #field),                                         \
        inner_field,                                                                \
        field                                                                       \
    )

#define LOG_CUSTOM_DEBUG() LOG(market_shops::logger::GetCtx().logging_level)

#define PARSE_FIELD(Struct, Name) value[#Name].As<decltype(Struct::Name)>()

#define THERE_ARE_MULTIPLE_VARIANT_MATCH_BY_DIFFERENT_PATHS

#define KALKANCRYPT_H

#define KC_DECL

#define KCST_PKCS12 0x00000001

#define KCST_KZIDCARD 0x00000002

#define KCST_KAZTOKEN 0x00000004

#define KCST_ETOKEN72K 0x00000008

#define KCST_JACARTA 0x00000010

#define KCST_X509CERT 0x00000020

#define KCST_AKEY 0x00000040

#define KCST_ETOKEN5110 0x00000080

#define KC_CERT_DER 0x00000101

#define KC_CERT_PEM 0x00000102

#define KC_CERT_B64 0x00000104

#define KC_CERT_CA 0x00000201

#define KC_CERT_INTERMEDIATE 0x00000202

#define KC_CERT_USER 0x00000204

#define KC_USE_NOTHING 0x00000401

#define KC_USE_CRL 0x00000402

#define KC_USE_OCSP 0x00000404

#define KC_XML_INCL_C14N 0x01000001

#define KC_XML_INCL_C14NCOMMENT 0x01000002

#define KC_XML_INCL_C14N11 0x01000004

#define KC_XML_INCL_C14N11COMMENT 0x01000008

#define KC_XML_EXCL_C14N 0x01000010

#define KC_XML_EXCL_C14NCOMMENT 0x01000020

#define KC_XMLC_INCL_C14N 0x01000040

#define KC_XMLC_INCL_C14NCOMMENT 0x01000080

#define KC_XMLC_INCL_C14N11 0x01000100

#define KC_XMLC_INCL_C14N11COMMENT 0x01000200

#define KC_XMLC_EXCL_C14N 0x01000400

#define KC_XMLC_EXCL_C14NCOMMENT 0x01000800

#define KC_CERTPROP_ISSUER_COUNTRYNAME 0x00000801

#define KC_CERTPROP_ISSUER_SOPN 0x00000802

#define KC_CERTPROP_ISSUER_LOCALITYNAME 0x00000803

#define KC_CERTPROP_ISSUER_ORG_NAME 0x00000804

#define KC_CERTPROP_ISSUER_ORGUNIT_NAME 0x00000805

#define KC_CERTPROP_ISSUER_COMMONNAME 0x00000806

#define KC_CERTPROP_SUBJECT_COUNTRYNAME 0x00000807

#define KC_CERTPROP_SUBJECT_SOPN 0x00000808

#define KC_CERTPROP_SUBJECT_LOCALITYNAME 0x00000809

#define KC_CERTPROP_SUBJECT_COMMONNAME 0x0000080a

#define KC_CERTPROP_SUBJECT_GIVENNAME 0x0000080b

#define KC_CERTPROP_SUBJECT_SURNAME 0x0000080c

#define KC_CERTPROP_SUBJECT_SERIALNUMBER 0x0000080d

#define KC_CERTPROP_SUBJECT_EMAIL 0x0000080e

#define KC_CERTPROP_SUBJECT_ORG_NAME 0x0000080f

#define KC_CERTPROP_SUBJECT_ORGUNIT_NAME 0x00000810

#define KC_CERTPROP_SUBJECT_BC 0x00000811

#define KC_CERTPROP_SUBJECT_DC 0x00000812

#define KC_CERTPROP_NOTBEFORE 0x00000813

#define KC_CERTPROP_NOTAFTER 0x00000814

#define KC_CERTPROP_KEY_USAGE 0x00000815

#define KC_CERTPROP_EXT_KEY_USAGE 0x00000816

#define KC_CERTPROP_AUTH_KEY_ID 0x00000817

#define KC_CERTPROP_SUBJ_KEY_ID 0x00000818

#define KC_CERTPROP_CERT_SN 0x00000819

#define KC_CERTPROP_ISSUER_DN 0x0000081a

#define KC_CERTPROP_SUBJECT_DN 0x0000081b

#define KC_CERTPROP_SIGNATURE_ALG 0x0000081c

#define KC_CERTPROP_PUBKEY 0x0000081d

#define KC_CERTPROP_POLICIES_ID 0x0000081e

#define KC_SIGN_DRAFT 0x00000001

#define KC_SIGN_CMS 0x00000002

#define KC_IN_PEM 0x00000004

#define KC_IN_DER 0x00000008

#define KC_IN_BASE64 0x00000010

#define KC_IN2_BASE64 0x00000020

#define KC_DETACHED_DATA 0x00000040

#define KC_WITH_CERT 0x00000080

#define KC_WITH_TIMESTAMP 0x00000100

#define KC_OUT_PEM 0x00000200

#define KC_OUT_DER 0x00000400

#define KC_OUT_BASE64 0x00000800

#define KC_PROXY_OFF 0x00001000

#define KC_PROXY_ON 0x00002000

#define KC_PROXY_AUTH 0x00004000

#define KC_IN_FILE 0x00008000

#define KC_NOCHECKCERTTIME 0x00010000

#define KC_HASH_SHA256 0x00020000

#define KC_HASH_GOST95 0x00040000

#define KC_GET_OCSP_RESPONSE 0x00080000

#define KCR_BASE 0x08F00000

#define KCR_OK 0x00000000

#define KCR_INIT_ERROR KCR_BASE + 0x00000001

#define KCR_ERROR_READ_PKCS12 KCR_BASE + 0x00000002

#define KCR_ERROR_OPEN_PKCS12 KCR_BASE + 0x00000003

#define KCR_INVALID_PROPID KCR_BASE + 0x00000004

#define KCR_BUFFER_TOO_SMALL KCR_BASE + 0x00000005

#define KCR_CERT_PARSE_ERROR KCR_BASE + 0x00000006

#define KCR_INVALID_FLAG KCR_BASE + 0x00000007

#define KCR_OPENFILEERR KCR_BASE + 0x00000008

#define KCR_INVALIDPASSWORD KCR_BASE + 0x00000009

#define KCR_CERTWRONGDATE KCR_BASE + 0x0000000a

#define KCR_CERTEXPIRED KCR_BASE + 0x0000000b

#define KCR_ISNOTCACERT KCR_BASE + 0x0000000c

#define KCR_MEMORY_ERROR KCR_BASE + 0x0000000d

#define KCR_CHECKCHAINERROR KCR_BASE + 0x0000000e

#define KCR_CACERTKEYUSAGEERROR KCR_BASE + 0x0000000f

#define KCR_VALIDTYPEERROR KCR_BASE + 0x00000010

#define KCR_BADCRLFORMAT KCR_BASE + 0x00000011

#define KCR_LOADCRLERROR KCR_BASE + 0x00000012

#define KCR_LOADCRLSERROR KCR_BASE + 0x00000013

#define KCR_UNKNOWN_ALG KCR_BASE + 0x00000015

#define KCR_KEYNOTFOUND KCR_BASE + 0x00000016

#define KCR_SIGN_INIT_ERROR KCR_BASE + 0x00000017

#define KCR_SIGN_ERROR KCR_BASE + 0x00000018

#define KCR_ENCODE_ERROR KCR_BASE + 0x00000019

#define KCR_INVALID_FLAGS KCR_BASE + 0x0000001a

#define KCR_CERTNOTFOUND KCR_BASE + 0x0000001b

#define KCR_VERIFYSIGNERROR KCR_BASE + 0x0000001c

#define KCR_BASE64_DECODE_ERROR KCR_BASE + 0x0000001d

#define KCR_UNKNOWN_CMS_FORMAT KCR_BASE + 0x0000001e

#define KCR_GETHASHERROR KCR_BASE + 0x0000001f

#define KCR_CA_CERT_NOT_FOUND KCR_BASE + 0x00000020

#define KCR_XMLSECINIT_ERROR KCR_BASE + 0x00000021

#define KCR_LOADTRUSTEDCERTSERR KCR_BASE + 0x00000022

#define KCR_SIGN_INVALID KCR_BASE + 0x00000023

#define KCR_NOSIGNFOUND KCR_BASE + 0x00000024

#define KCR_DECODE_ERROR KCR_BASE + 0x00000025

#define KCR_XMLPARSEERROR KCR_BASE + 0x00000026

#define KCR_XMLADDIDERROR KCR_BASE + 0x00000027

#define KCR_XMLINTERNALERROR KCR_BASE + 0x00000028

#define KCR_XMLSETSIGNERROR KCR_BASE + 0x00000029

#define KCR_OPENSSLERROR KCR_BASE + 0x0000002a

#define KCR_ENGINE_INITERR KCR_BASE + 0x0000002b

#define KCR_NOTOKENFOUND KCR_BASE + 0x0000002c

#define KCR_OCSP_ADDCERTERR KCR_BASE + 0x0000002d

#define KCR_OCSP_PARSEURLERR KCR_BASE + 0x0000002e

#define KCR_OCSP_ADDHOSTERR KCR_BASE + 0x0000002f

#define KCR_OCSP_REQERR KCR_BASE + 0x00000030

#define KCR_OCSP_CONNECTIONERR KCR_BASE + 0x00000031

#define KCR_VERIFY_NODATA KCR_BASE + 0x00000032

#define KCR_IDATTR_NOTFOUND KCR_BASE + 0x00000033

#define KCR_IDRANGE KCR_BASE + 0x00000034

#define KCR_XMLKEYDUPERROR KCR_BASE + 0x00000035

#define KCR_XMLKEYCREATEERROR KCR_BASE + 0x00000036

#define KCR_READERNOTFOUND KCR_BASE + 0x00000037

#define KCR_GETCERTPROPERR KCR_BASE + 0x00000038

#define KCR_SIGNFORMMAT KCR_BASE + 0x00000039

#define KCR_INDATAFORMAT KCR_BASE + 0x0000003a

#define KCR_OUTDATAFORMAT KCR_BASE + 0x0000003b

#define KCR_VERIFY_INIT_ERROR KCR_BASE + 0x0000003c

#define KCR_VERIFY_ERROR KCR_BASE + 0x0000003d

#define KCR_HASH_ERROR KCR_BASE + 0x0000003e

#define KCR_SIGNHASH_ERROR KCR_BASE + 0x0000003f

#define KCR_CACERTNOTFOUND KCR_BASE + 0x00000040

#define KCR_CERTTIMEINVALID KCR_BASE + 0x00000042

#define KCR_CONVERTERROR KCR_BASE + 0x00000043

#define KCR_TSACREATEQUERY KCR_BASE + 0x00000044

#define KCR_CREATEOBJ KCR_BASE + 0x00000045

#define KCR_CREATENONCE KCR_BASE + 0x00000046

#define KCR_HTTPERROR KCR_BASE + 0x00000047

#define KCR_CADESBES_FAILED KCR_BASE + 0x00000048

#define KCR_CADEST_FAILED KCR_BASE + 0x00000049

#define KCR_NOTSATOKEN KCR_BASE + 0x0000004a

#define KCR_INVALID_DIGEST_LEN KCR_BASE + 0x0000004b

#define KCR_GENRANDERROR KCR_BASE + 0x0000004c

#define KCR_SOAPNSERROR KCR_BASE + 0x0000004d

#define KCR_GETPUBKEY KCR_BASE + 0x0000004e

#define KCR_GETCERTINFO KCR_BASE + 0x0000004f

#define KCR_FILEREADERROR KCR_BASE + 0x00000050

#define KCR_CHECKERROR KCR_BASE + 0x00000051

#define KCR_ZIPEXTRACTERR KCR_BASE + 0x00000052

#define KCR_NOMANIFESTFILE KCR_BASE + 0x00000053

#define KCR_LIBRARYNOTINITIALIZED KCR_BASE + 0x00000101

#define KCR_ENGINELOADERR KCR_BASE + 0x00000200

#define KCR_PARAM_ERROR KCR_BASE + 0x00000300

#define KCR_CERT_STATUS_OK KCR_BASE + 0x00000400

#define KCR_CERT_STATUS_REVOKED KCR_BASE + 0x00000401

#define KCR_CERT_STATUS_UNKNOWN KCR_BASE + 0x00000402

#define ETA_UPDATE_WITH_DUP(val)                                                                               \
    ::stq_agent::common::names::mongo::stq::kEta, (val), ::stq_agent::common::names::mongo::stq::kInfoWithEta, \
        (GetEtaWithInfo(val))

#define RETHROW_IF_NEEDED(do_rethrow) \
    if (do_rethrow) throw

#define LOG_ERROR_IF_FAILED(condition, message)                                                    \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            const auto err_str = ::fmt::format("Validation ({}) failed: {}", #condition, message); \
            LOG_ERROR() << err_str;                                                                \
        }                                                                                          \
    } while (0)

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 17

#define ASSERT_PROTO(lhs, rhs) ASSERT_TRUE(supportai_core::core::tests::Compare(lhs, rhs))

#define ASSERT_PROTO_WITH_IGNORE(lhs, rhs, vector) ASSERT_TRUE(supportai_core::core::tests::Compare(lhs, rhs, vector))

#define PARSE_JSON_FIELD(field_name, json) field_name = json[#field_name].As<decltype(result.field_name)>()

#define LOG_TAG(TagEventT) \
    LogTag<TagEventT>(yt_logger, provider_name, entity_name, entity_type, tag_name, now_timestring, ttl, active)

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
        static const std::string GetMetaType() { return experiments3::ExperimentName::kName; }                  \
    };

#define COMMIT_ERROR_ENUM_MAP(XX) XX(ORDER_NOT_FOUND, "ORDER_NOT_FOUND")

#define ORDER_SERVICE_TYPE_ENUM_MAP(XX) \
    XX(Cargo, "cargo")                  \
    XX(Taxi, "taxi")

#define CODEGEN_TO_PG_JSONB(codegen_type)                                                         \
    namespace storages::postgres::io {                                                            \
    namespace traits {                                                                            \
    template <>                                                                                   \
    struct Input<codegen_type> {                                                                  \
        using Converter = ultima_mode::utils::json::JsonPgConverter<codegen_type>;                \
        using type = TransformParser<Converter::UserType, Converter::PostgresType, Converter>;    \
    };                                                                                            \
    template <>                                                                                   \
    struct Output<codegen_type> {                                                                 \
        using Converter = ultima_mode::utils::json::JsonPgConverter<codegen_type>;                \
        using type = TransformFormatter<Converter::UserType, Converter::PostgresType, Converter>; \
    };                                                                                            \
    }                                                                                             \
    template <>                                                                                   \
    struct CppToSystemPg<codegen_type> : PredefinedOid<PredefinedOids::kJsonb> {};                \
    }

#define MUST_HAVE_VALUE(field)                                                \
    do {                                                                      \
        if (!field.has_value()) {                                             \
            LOG_ERROR() << "place " << place.id << " missing field: " #field; \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define BETTER(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kBetter) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Better";

#define WORST(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kWorst) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Worst";

#define EQUAL(func, lhs, rhs) \
    EXPECT_EQ(func(lhs, rhs), CompareResult::kEqual) << #func "(\n\t" #lhs ",\n\t" #rhs "\n) != Equal";

#define BOOST_DISABLE_PRAGMA_MESSAGE

#define EXPECT_EQ(...)                       \
    do {                                     \
        try {                                \
            ExpectEq(__VA_ARGS__);           \
        } catch (const std::exception& ex) { \
            LOG_ERROR() << ex.what();        \
            throw;                           \
        }                                    \
    } while (0)

#define EXPECT_NOTHROW(x)                    \
    do {                                     \
        try {                                \
            x;                               \
        } catch (const std::exception& ex) { \
            LOG_ERROR() << ex.what();        \
            throw;                           \
        }                                    \
    } while (0)

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

#define SHOULD_CANCEL_POINT                   \
    if (engine::current_task::ShouldCancel()) \
    throw utils::should_cancel::ShouldCancelException()  // ";" has to be in code

#define SLEEP_WITH_SHOULD_CANCEL_POINT(interval) \
    engine::InterruptibleSleepFor(interval);     \
    SHOULD_CANCEL_POINT

#define UNORDERED_REQUIREMENTS_EQUAL_FIELDS(z, data, field) &&a.field == b.field

#define UNORDERED_REQUIREMENTS_OPERATOR_EQUAL(Type, fields)                                \
    inline bool operator==(const Type& a, const Type& b) noexcept {                        \
        return true BOOST_PP_SEQ_FOR_EACH(UNORDERED_REQUIREMENTS_EQUAL_FIELDS, ~, fields); \
    }

#define UNORDERED_REQUIREMENTS_COMBINE_FIELD(z, data, field) boost::hash_combine(seed, value.field);

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

#define UNORDERED_REQUIREMENTS(nspace, Type, fields)    \
    namespace nspace {                                  \
    UNORDERED_REQUIREMENTS_OPERATOR_EQUAL(Type, fields) \
    }                                                   \
    UNORDERED_REQUIREMENTS_STD_HASH(nspace::Type, fields)

#define DECL_SPAN_KEY(T, KEY)               \
    template <>                             \
    struct SpanKeyInfo<T> {                 \
        static constexpr auto kKey = (KEY); \
    };

#define DECL_YABX_API_ERROR(cls, name)                                                                \
    struct cls {                                                                                      \
        static constexpr auto kName = #name;                                                          \
        int32_t code = {};                                                                            \
        std::string message = {};                                                                     \
        constexpr std::string FormatMessage() const {                                                 \
            return fmt::format("Api responded with {}, code: {}, message: {}", kName, code, message); \
        }                                                                                             \
    };

#define YW_EXPECT(_condition, _message)                                                          \
    do {                                                                                         \
        if (_condition) {                                                                        \
        } else {                                                                                 \
            throw ::yango_wallet::utils::ExpectError(_message, std::source_location::current()); \
        }                                                                                        \
    } while (false)

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

#define FORMAT_IFDEF_FIXTURE_HPP

#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((noinline, flatten))

#define FORMAT_USERVER_PROTECTED_ATTR __attribute__((always_inline, flatten))

#define USERVER_IMPL_NODEBUG __attribute__((__nodebug__))

#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__nodebug__, __always_inline__))

#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__always_inline__))

#define FORMAT_USERVER_CONST

#define FORMAT_USERVER_CONST const

#define CURL_FORMAT_USERVER_NAMESPACE fixture::

#define CURL_FORMAT_USERVER_NAMESPACE

#define СООБЩЕНИЕ Use("01234567890123456789é𝄞")

#define JOIN a+""+b+c

#define FORMAT_EOF_VALUE 1

#define FORMAT_EOF_PAIR \
 (sizeof("prefix") - 1, sizeof(">") - 1)

#define FORMAT_EOF_BODY() \
 do {} while(false) \

#define FORMAT_FIXTURE_SUM(firstValue, secondValue, thirdValue) \
    ((firstValue) + \
        (secondValue) + \
        (thirdValue) + \
        (firstValue) + \
        (secondValue) + \
        (thirdValue))

#define FORMAT_FIXTURE_SHORT_MACRO(value) (value)

#define FORMAT_FIXTURE_MUCH_LONGER_MACRO(value) (value)

#define FORMAT_FIXTURE_STATEMENT_ARGUMENT(MethodName, ParamType, ParamName, ...) \
auto& MethodName(ParamType ParamName) { __VA_ARGS__ return *this; }

#define FORMAT_FIXTURE_STRUCTURED_LAMBDA [](){a();b();}

#define FORMAT_FIXTURE_DECLARE_OPTION(name,type) void name(type)

#define FORMAT_FIXTURE_LOAD_OPTIONAL(function,name) \
function=reinterpret_cast<decltype(function)>(GetProcAddress(module_,name))

#define FORMAT_FIXTURE_ITEMS(X) \
X(Alpha,"alpha") X(Beta,"beta") X(Gamma,"gamma")

#define FORMAT_FIXTURE_ENUM_ITEMS(X) \
X(First,"first") X(Second,"second")

#define FORMAT_FIXTURE_COMMENT_CONTINUATION(callback) \
    callback(); \
    /* cold testing path: */ \
    callback();

#define FORMAT_FIXTURE_TOKEN_PASTE(prefix,suffix) \
prefix ## suffix

#define FORMAT_PASTED_FN(name) inline int Get##name##Value(){return 0;}

#define FORMAT_PASTED_INIT(name) {(name),Get##name##Value()}

#define FORMAT_PASTED_NUMBER(suffix) 10 ## suffix

#define FORMAT_FIXTURE_STRINGIZE(value) \
#value

#define FORMAT_FIXTURE_FILEPATH FORMAT_NAMESPACE::logging::impl::CutFilePath(__builtin_FILE())

#define FORMAT_FIXTURE_REGISTER_TYPE(Type,Index) \
constexpr std::size_t TypeToId(FormatFixtureIdentity<Type>) noexcept{return Index;} \
constexpr Type IdToType(FormatFixtureSize<Index>) noexcept{return FormatFixtureConstruct<Type>();}

#define ENUM_STRING_DECLARE(EnumType, ItemsMacro) \
    enum class EnumType{ItemsMacro( \
        ENUM_STRING_DECLARE_ENUMERATOR \
    )}; template <> struct EnumStringTraits<EnumType>{static constexpr auto names = std::to_array<std::string_view>({ItemsMacro(ENUM_STRING_DECLARE_NAME)}); }

#define FORMAT_FIXTURE_TEMP_MACRO(value) (value)

#define FORMAT_FIXTURE_METHOD_MARKER(value) (value)

#define COMMENTED_ARGUMENTS(value) Configure(/* mode = */"default",/* options = */{},/* value = */value)

#define COMMENTED_SUM(first, second) \
    (first /* first term */ \
        + second)

#define COMMENTED_STREAM(out, value) \
    out << /* insertion */ \
        value

#define FORMAT_NAMESPACE_TRAITS(Type) namespace format_macro { namespace detail { template<> struct Traits<Type> {using type=Type;}; } }

#define FORMAT_NESTED_NAMESPACE_TRAITS(Type) namespace format_macro::nested { inline namespace version { template<> struct Traits<Type> {static_assert(Check<Type>());}; } }

#define FORMAT_MIXED_NAMESPACE_DECLARATIONS(Type) namespace format_macro {Type Get();} void After();

#define FORMAT_ANONYMOUS_NAMESPACE(Type) namespace {namespace detail {Type value;}}

#define FORMAT_ALIGN_LONG_LINE() void LongMacroLine() {Use("This indivisible string literal deliberately exceeds the configured column limit and must not push the other continuation backslashes to the right."); Short();}

#define FORMAT_ALIGN_RAW_STRING() void RawMacroLine() {Use(R"text(raw string content ending in a backslash \
this line is still inside the raw string)text"); Short();}

#define FORMAT_PRIMITIVE_DECLARATION int value;

#define FORMAT_PRIMITIVE_DECL_SEQUENCE int first; unsigned long second; T named;

#define FORMAT_PRIMITIVE_FUNCTIONS void F(); int G(int value);

#define FORMAT_RECURSIVE_PRIMITIVE_FUNCTIONS int (*Factory())(); int (&Array())[3];

#define FORMAT_PRIMITIVE_INITIALIZER static const int value = Make();

#define FORMAT_DECLARATION_NAMESPACE_SEQUENCE void Before(); namespace format_macro {int value;} void After();

#define FORMAT_QUALIFIED_PROTOTYPES(type) detail::Value First(const type& value); ::QualifiedMacroFunctions::detail::Value& operator<<(::QualifiedMacroFunctions::detail::Value& value,const type& arg);

#define FORMAT_QUALIFIED_SINGLE detail::Value First() { return {}; }

#define FORMAT_QUALIFIED_SEQUENCE detail::Value Second() { return {}; } int Third() { return 3; } detail::Value Fourth() { return {}; }

#define FORMAT_QUALIFIED_CONSTEXPR constexpr detail::Value Fifth() { return {}; } constexpr int Sixth() { return 6; } constexpr detail::Value Seventh() { return {}; }

#define FORMAT_QUALIFIED_MODIFIERS [[nodiscard]] inline const detail::Value& Ref(const detail::Value& value) { return value; }

#define FORMAT_QUALIFIED_TEMPLATE template<class T> detail::Value Convert(T value) { return {static_cast<int>(value)}; }

#define FORMAT_QUALIFIED_CONSTANT constexpr detail::Value constant{};

#define FORMAT_QUALIFIED_VARIABLE_NAME(name) name

#define FORMAT_QUALIFIED_HEADER static detail::Value FORMAT_QUALIFIED_VARIABLE_NAME(variable)

#define FORMAT_QUALIFIED_RECURSIVE detail::Value (*Factory())() { return First; }

#define FORMAT_MACRO_NESTED_DECLARATORS int (((*Factory())))(int); int (((&Array())))[3];

#define FORMAT_MACRO_IF(value) if(value) { result+=value; }

#define FORMAT_MACRO_FOR(values) for(auto value:values) { result+=value; }

#define FORMAT_MACRO_BLOCK(value) { int local=value; result+=local; }

#define FORMAT_MACRO_MIXED(value) int local=value; if(local) { result+=local; } ++result;

#define FORMAT_MACRO_WHILE(value) while(value>0) { result+=value; --value; }

#define FORMAT_MACRO_SWITCH(value) switch(value) { case 1: ++result; break; default: result+=2; }

#define FORMAT_MACRO_DO(value) do { result+=value; } while(false)

#define FORMAT_MACRO_UNBRACED_DO(value) do result+=value; while(false)

// Macro definitions preserve bracing at every nesting level.
#define FORMAT_OPEN_LOG(level) for(bool once=true;once;once=false) LOG(level)
void OpenLogUse() { FORMAT_OPEN_LOG(Info) << "hello"; }
#define FORMAT_OBJECT_CONTROL if(ready) Run();
#define FORMAT_UNBRACED_CONTROL(value) if(value) Run(); else Stop(); while(value) Step(); for(auto item:values) Use(item); switch(value) case 1: break;
#define FORMAT_EMPTY_CONTROL for(;;); while(ready); do;while(ready)
#define FORMAT_ELSE_CONTROL if(ready) { Run(); } else { if(pending) Wait(); else Stop(); }
#define FORMAT_NESTED_CONTROL void Generated() { if(ready) Run(); } auto callback=[] { while(ready) Run(); };
#define FORMAT_ARGUMENT_CONTROL APPLY(if(ready) Run(); else { if(pending) Wait(); })
void BesideMacroDefinition() {
#define FORMAT_LOCAL_CONTROL(value) if(value) Run();
if(ready) Run(); else { if(pending) Wait(); }
}

#define FORMAT_MACRO_COMPLETE_DO(value) do { result+=value; } while(false);

#define FORMAT_MACRO_FINAL_DO(value) ++result; do { result+=value; } while(false)

#define FORMAT_MACRO_TRY(value) try { throw value; } catch(int amount) { result+=amount; }

#define SELECT_VALUE(record, field) ((record).Has##field() ? (record).field : k_##field)

#define READ_VALUE(record, field) ((record)->get_##field())

#define ASSIGN_VALUE(record, field, result) (record).field##ue = result

#define CONVERT_VALUE(record, field, Type) ((record).template as_##field<Type>())

#define CHAIN_VALUE(record, part, rest) ((record).get_##part##rest())

#define COMPUTE_VALUE(field) (k_##field + 2 * k_##field)

#define FORMAT_PASTED_DECL_FUNCTION(Suffix) constexpr int Get##Suffix() { return 7; }

#define FORMAT_PASTED_DECL_VARIABLE(Suffix) constexpr int k##Suffix = 9;

#define FORMAT_PASTED_DECL_RESULT(Suffix) Result##Suffix Build##Suffix() { return {}; }

#define FORMAT_PASTED_DECL_QUALIFIED(Suffix) Group##Suffix::Result Qualified##Suffix() { return {}; }

#define FORMAT_PASTED_DECL_TEMPLATE(Suffix) template<class T> T Convert##Suffix(T value) { return value; }

#define FORMAT_PASTED_DECL_POINTER(Suffix) int (*Pointer##Suffix())(int) { return &Convert##Suffix<int>; }

#define FORMAT_TYPE_DECLARATIONS(Name) \
struct Name##Tag {}; \
class Name##Forward; \
union Name##Union {int value; double other;}; \
using Name = Name##Tag; \
typedef Name Name##Alias; \
using Name##Callback = int (*)(int); \
using Name##Function = int(int);

#define FORMAT_TYPE_MEMBER_ALIAS(Name) using Name = int Owner::*;

#define FORMAT_TYPE_NAMESPACE(Name) namespace Name {using Number = int;} namespace Name##Alias = Name; using Name::Number;

#define FORMAT_TYPE_TEMPLATE(Name) template<class T> struct Name {T value;}; template<class T> using Name##Alias = Name<T>;

#define FORMAT_TYPE_CONCEPT(Name) template<class T> concept Name = sizeof(T) > 0;

#define FORMAT_TYPE_EXTERN(Name) extern "C" {int Name(int);}

#define FORMAT_TYPE_INSTANTIATION(Name) template struct Name<int>;

#define FORMAT_TYPE_CLASS(Name) class Name##Derived final : public Owner {public: Name##Derived(int value) { member=value; }};

#define FORMAT_STATEMENT_PREFIX_THROW throw

#define FORMAT_STATEMENT_PREFIX_DISCARD (void)

#define FORMAT_STATEMENT_PREFIX_FOR(count) for (int index = 0; index < count; ++index)

#define FORMAT_STATEMENT_PREFIX_TRACE() if (true)

#define FORMAT_SEMILESS_INC(value) ++value;

#define FORMAT_SEMILESS_RETURN(value) return value;

#define FORMAT_SEMILESS_DECLARE(name) constexpr int name=2;

#define FORMAT_SEMILESS_RELAY(value) FORMAT_SEMILESS_INC(value) FORMAT_SEMILESS_INC(value)

#define FORMAT_SEMILESS_TEXT() "part"

#define FORMAT_SEMILESS_CONCAT() FORMAT_SEMILESS_TEXT() FORMAT_SEMILESS_TEXT() "end"

#define FORMAT_ANON_ENUM(name) enum : bool {name = true};

#define FORMAT_ANON_ATTR_ENUM(name) enum [[maybe_unused]] : unsigned {name = 3};

#define FORMAT_ENUM_ATTR(Name) enum Name {Old##Name [[deprecated]], Current##Name [[maybe_unused]] = 1};

#define FORMAT_COMMA_STRINGS "one", "two", "three"

#define FORMAT_COMMA_VALUES(value) 1, (value), 3, ((value) + 1)

#define FORMAT_COMMA_TRAILING(value) (value), ((value) + 1),

#define FORMAT_COMMA_EXPRESSIONS(flag, value) (flag) ? (value) : 1, (value) + 2

#define FORMAT_COMMA_SINGLE(value) (value),

#define FORMAT_COMMA_PARENTHESIZED(value) ((value), ((value) + 1))

#define FORMAT_COMMA_QUALIFIED data::Text, data::Box<int>, data::Box<data::Box<bool>>

#define FORMAT_DECLARATOR_MODIFIER

#define FORMAT_DECLARATOR_ATTRIBUTE(...)

#define FORMAT_INITIALIZER_FIELD(name, value) .name = (value)

#define FORMAT_INITIALIZER_FIELDS(first_value, second_value) .first = (first_value), .second = (second_value)

#define FORMAT_INITIALIZER_BRACED(name, ...) .name{__VA_ARGS__}

#define FORMAT_INITIALIZER_RECORDS(value) {(value), (value) + 1}, {(value) + 2, (value) + 3},

#define FORMAT_INITIALIZER_NESTED(value) .pair = {FORMAT_INITIALIZER_FIELDS(value, (value) + 1)},

#define FORMAT_INITIALIZER_PASTE(name, value) .name##st = (value),

#define FORMAT_ATTRIBUTE_GNU_INLINE __attribute__((always_inline, pure))

#define FORMAT_ATTRIBUTE_GNU_COLD __attribute__((noinline)) __attribute__((cold))

#define FORMAT_ATTRIBUTE_GNU_ALTERNATE __attribute((unused))

#define FORMAT_ATTRIBUTE_MIXED [[maybe_unused]] __attribute__((unused))

#define FORMAT_ATTRIBUTE_MS_NOINLINE __declspec(noinline)

#define FORMAT_TOKEN_ITEM(name, ...) static constexpr int name = 1;

#define FORMAT_TOKEN_STATEMENT(...) Consume(1);

#define FORMAT_TOKEN_WRAPPER(name, ...) FORMAT_TOKEN_ITEM(name, __VA_ARGS__)

#define FORMAT_PARAMETER_SUFFIX [[maybe_unused]]

#define FORMAT_PARAMETER_SUFFIX_CALL(...) [[maybe_unused]]

#define FORMAT_TYPE_METHOD(Return, Name, Parameters, Qualifiers)

#define FORMAT_SEMILESS_RECORD_FUNCTION(Name) auto Name() -> struct Record { return {}; }

#define FORMAT_STATEMENT_DECLARATIONS(Statement, Tag) do { Statement; } while (false)

#define FORMAT_TYPE_SIZE(Type) sizeof(Type)

#define FORMAT_TYPE_WORDS(...) ((void)0)

#define FORMAT_TYPE_ALIAS(Name, ...) using Name = __VA_ARGS__;

#define FORMAT_LIST_VALUE 1

#define FORMAT_LIST_VALUE 2

#define FORMAT_LIST_VALUE 3

#define FORMAT_LIST_VALUE 4

#define FORMAT_LIST_OPERAND 2

#define FORMAT_LIST_PARAMETER Integer

#define FORMAT_LIST_ARGUMENT 3

#define FORMAT_LIST_ARGUMENT 1

#define FORMAT_LIST_RAW_TEXT R"tag(
#define UNEXPANDED 3
#undef UNEXPANDED
)tag"

#define FORMAT_TYPE_RECORD(name) struct name { using Value=int; Value value; }

#define FORMAT_TYPE_CHOICE(name) union name { int number; char letter; }

#define FORMAT_TYPE_ENUM(name) enum class name : unsigned { First, Second }

#define FORMAT_PRIMITIVE_TYPE unsigned long

#define FORMAT_TYPE_TAG struct NamedType { int value; }

#define FORMAT_SEMILESS_NAMESPACE_COMMENT() namespace Outer { namespace Inner { int value=1; } /* Inner */ \
} /* Outer */

#define FORMAT_LAMBDA_CALLBACK(value) do { Invoke("a callback argument that makes the invocation exceed the configured line width",value,[](int argument) { if (argument) { ++argument; } }); } while(false)

#define FORMAT_SEMILESS_GENERATE(X) X(First, 2) X(Second, 3)

#define FORMAT_SEMILESS_EMPTY(X)

#define FORMAT_SEMILESS_SINGLE(X) X(Third, 4)

#define FORMAT_LIST_ENUM(name,value) name=value,

#define FORMAT_LIST_ENTRY(name,value) value,

#define FORMAT_ITEM_LIST 6,7,

#define FORMAT_ITEM_EMPTY

#define FORMAT_ITEM_VALUE 9

#define FORMAT_SEMILESS_VALUE() 10

#define FORMAT_ITEM_ENUM One,Two,

#define FORMAT_SEMILESS_TEXT() "x"

#define FORMAT_TYPE_GENERATED(name) enum class name { FORMAT_ITEM_ENUM Last }

#define FORMAT_ITEM_FIELD int first=1;

#define FORMAT_ITEM_EXTRA int second=2;

#define FORMAT_ITEM_EMPTY

#define FORMAT_ITEM_COMBINED FORMAT_ITEM_FIELD FORMAT_ITEM_EXTRA

#define FORMAT_ITEM_UNION_FIELD int number;

#define FORMAT_SEMILESS_DECLARE_GENERATED(Type) struct Type { FORMAT_ITEM_COMBINED int last=3; };

#define FORMAT_BARE_INCREMENT(Value) ((Value)+1)

#define FORMAT_TYPE_FORWARD_BARE(Value) \
FORMAT_BARE_INCREMENT(Value)

#define FORMAT_TYPE_TEMPLATE_HEADER(Name) template<class T> T Name(T value)

#define FORMAT_TYPE_SPECIALIZED_HEADER(Name) template<> int Name<int>(int value)

#define FORMAT_TYPE_NESTED_HEADER() template<class T> template<class U> U Values<T>::Convert(U value)

#define FORMAT_TYPE_CONSTRAINED_HEADER(Name) template<class T> requires (sizeof(T) > 0) T Name(T value)

#define FORMAT_TYPE_CONSTRAINED_SUFFIX(Name) template<class T> T Name(T value) requires (sizeof(T) > 0)

#define FORMAT_TYPE_REFERENCE_HEADER(Name) template<class T> T& Name(T& value)

#define FORMAT_TYPE_FACTORY_HEADER(Name) template<class T> T (*Name())(T)

#define FORMAT_TYPE_TRAILING_HEADER(Name) template<class T> auto Name(T* value) -> T*

#define FORMAT_TYPE_ARRAY_HEADER(Name) template<class T, unsigned N> T (&Name(T (&value)[N]))[N]

#define FORMAT_TYPE_VOID_HEADER(Name) template<class T> void Name(const T&)

#define FORMAT_TYPE_SEQUENCE_HEADER(Name) struct Tag {}; using Alias=Tag; void Declared(); template<class T> T Name(T value)

#define FORMAT_TYPE_VARIABLE(Name) template<class T> constexpr T Name=42

#define FORMAT_JOIN_INNER(Left, Right) Left##Right

#define FORMAT_JOIN(Left, Right) FORMAT_JOIN_INNER(Left, Right)

#define FORMAT_TYPE_TRACE(Message) const ::TemplateHeaderMacros::Details::Trace FORMAT_JOIN(trace_, __LINE__)(__FILE__, __LINE__, (Message))

#define FORMAT_GUARDED_NAMESPACE_ENABLED 1

#define FORMAT_GUARDED_NAMESPACE_SEEN

#define FORMAT_FIXTURE_LOGICAL_TAIL(Left,Right) && ((Left)==(Right))

#define FORMAT_FIXTURE_ARITHMETIC_TAIL(Value) + (Value)

#define FORMAT_CONTINUATION_COMPARE(Left,Right) FORMAT_FIXTURE_LOGICAL_TAIL(Left,Right)

#define FORMAT_CONTINUATION_ADD(Value) FORMAT_FIXTURE_ARITHMETIC_TAIL(Value)

#define FORMAT_CONTINUATION_TRUE_TAIL FORMAT_CONTINUATION_COMPARE(1,1)

#define FORMAT_TOKEN_COMPARE(Callback,Field) Callback(a.Field,b.Field)

#define FORMAT_SEMILESS_EQUAL(Type) inline bool operator==(const Type& a,const Type& b)noexcept{return true FORMAT_TOKEN_COMPARE(FORMAT_CONTINUATION_COMPARE,first) FORMAT_TOKEN_COMPARE(FORMAT_CONTINUATION_COMPARE,second);}

#define FORMAT_ITEM_LIST_VALUES 3,4,

#define FORMAT_ITEM_LIST_EMPTY

#define FORMAT_SEMILESS_LIST_VALUES() 5,6,

#define FORMAT_SEMILESS_LIST_EMPTY()

#define FORMAT_USERVER_DO_WHILE(flag) \
    do { \
        if (flag) break; \
        UseFlag(flag); \
    } while (false)

#define USERVER_IMPL_FORCE_INLINE __attribute__((always_inline)) inline

#define LOG_FORMAT_USERVER_LIMITED(logger, level, ...) \
    if (const RateLimiter limiter{[]() -> RateLimitData& { \
            static RateLimitData data; \
            return data; \
        }()}; \
        !limiter.ShouldLog()) \
    { \
    } else \
        LOG_TO((logger), (level), __VA_ARGS__) << limiter

#define FORMAT_USERVER_COMPLEX_OPTION(FUNCTION_NAME, OPTION_TYPE) \
    inline void FUNCTION_NAME(OPTION_TYPE arg) { \
        UseOption(arg, PP_STRINGIZE(FUNCTION_NAME)); \
    }

#define FORMAT_USERVER_HASH_JOIN(FUNCTION_NAME) \
private: \
    inline void FUNCTION_NAME##_impl() {} \
public: \
    static constexpr bool is_##FUNCTION_NAME##_available = true

#define FORMAT_USERVER_EXPECT_TRY(cmd) \
    try { \
        cmd; \
    } catch (const Error& error) { \
        EXPECT_EQ(error.Code(), ErrorCode::kExpected); \
    }

#define BENCHMARK_THREAD_ARGS ->Arg(2)->Arg(4)

#define IMPL_UTEST_FORMAT_USERVER(name) \
    TestLauncher<::testing::Test>::RunTest< \
        name>();                         \
    struct FormatUserverForceSemicolon

#define FORMAT_EMPTY_TEST(suite, name) void suite()

#define FORWARD_EMPTY_ARGUMENTS(value) TEST_COMMAND(value,, /* empty */)

#define NESTED_EMPTY_ARGUMENTS(value) TEST_COMMAND(, TEST_COMMAND(value,,), TEST_COMMAND(TEST_COMMAND(,)),)

#define FORMAT_CALL_MACRO_ALIAS TEST_COMMAND

#define FORMAT_USERVER_COMPLETE_STATEMENT(value) ++value;

// Raw continuation alignment preserves fragments and each definition's own alignment group.
#define FORMAT_RAW_ALIGN_MEMBERS(T)              \
    private: \
        T& Borrow(Owner& owner);         \
    public:                           \
        using Base::Base

#define FORMAT_RAW_ALIGN_OVER_LIMIT() \
    )                                                                                                  \
    ThisIndivisiblePreprocessingTokenIsIntentionallyLongerThanTheColumnLimitAndMustNotPushOtherContinuationBackslashesPastTheirOwnAlignmentColumn       \
                   \
    short_value \
    ThisFinalLineIsAlsoDeliberatelyLongerThanTheColumnLimitAndMustNotParticipateInTheAlignmentOfAnyPrecedingContinuationBackslashes

// Literal-internal backslashes and indentation stay intact; surrounding continuations align.
#define FORMAT_RAW_ALIGN_LITERALS() \
    ) u8R"tag(first literal line \
                another literal line)tag"         \
    "first string line\
second string line" \
    last_value

// Preserve token-joining splices while aligning independent continuation lines.
#define FORMAT_RAW_ALIGN_TOKEN_JOIN() \
    namespace Joined\
Name {        \
    value +\
+; \
    public:

// Quotes in a continued comment do not start literals.
#define FORMAT_RAW_ALIGN_COMMENT() \
    ) // " and ' are comment text \
        continued comment text

// Punctuation separated by splices still aligns when the source omits padding.
#define FORMAT_RAW_ALIGN_NO_SPACE() \
)\
{\
value;\
}\
end

// Outdented case-body braces retain the macro continuation indentation.
#define FORMAT_MACRO_CASE_BODY(value) switch(value){case 0:{InitializeStep();UpdateStep();break;}default:break;}

// Blank continuation separators retain the ordinary source-item rules.
#define FORMAT_BLANK_STEPS(x) \
    First(x); \
    \
    \
    Second(x);
#define FORMAT_BLANK_DECLARATIONS \
    namespace generated { \
        struct First {}; \
        \
        struct Second {}; \
        \
    }
#define FORMAT_BLANK_CALLS(X) \
    X(First) \
    \
    X(Second)
#define FORMAT_BLANK_LIST \
    {First, \
     \
     Second}
#define FORMAT_CONTINUED_EXPRESSION(value) \
    First( \
        value \
    ) + \
    Second(value)

// Comma-separated replacement fragments expose element boundaries to the solver.
#define FORMAT_FRAGMENT_COMPACT first, second
#define FORMAT_FRAGMENT_TYPES namespace_name::String,namespace_name::Optional<namespace_name::Integer>,namespace_name::Optional<namespace_name::Boolean>,namespace_name::String
#define FORMAT_FRAGMENT_NAMES "first_parameter_with_a_descriptive_name","second_parameter_with_a_descriptive_name","third_parameter_with_a_descriptive_name","fourth_parameter_with_a_descriptive_name"
#define FORMAT_FRAGMENT_TRAILING FirstValueWithADeliberatelyLongName,SecondValueWithADeliberatelyLongName,ThirdValueWithADeliberatelyLongName,
#define FORMAT_FRAGMENT_COMMENT first, /* first item */ \
    second, \
    /* next item */ \
    third
#define FORMAT_FRAGMENT_BLANK first, \
    \
    second
#define FORMAT_FRAGMENT_NESTED Call(first,second),Type<First,Second>{first,second},(first,second)

// Trailing standalone comments remain in their structured macro replacement.
#define TRAILING_MACRO_COMMENT() \
    Call(); \
    /**/
#define TRAILING_MACRO_BLOCK_COMMENT() \
    if (ready) { Call(); } \
    /* first */ \
    /* second */
void WithTrailingMacroComment() {
#define LOCAL_MACRO_COMMENT() \
    Call(); \
    // marker
    LOCAL_MACRO_COMMENT();
}

// Macro arguments may contain declaration fragments and bare modifiers.
DEFINE_BINARY_PROTO_FUZZER(const TPackUnpackCase& value) { Exercise(value); }
Y_DECLARE_OUT_SPEC(inline, NType::TValue, stream, value) { stream << value; }
void MacroDeclarationArguments() {
    ASSIGN_OR_RAISE(auto result, Compute());
}

// A definition following a completed function keeps its structured replacement.
namespace GeneratedIdentifiers {
int Before() { return 1; }
#define DEFINE_IDENTIFIER(object)\
TGuid Generate##object##Id() \
{ \
 return GenerateId(EObjectType::object); \
} \

DEFINE_IDENTIFIER(Chunk)
}

// A final standalone comment must keep the preceding expression in the macro.
#define FOREACH_DESTINATION_TYPE(MACRO, ...) \
    MACRO(__VA_ARGS__, OT_VARIABLE)          \
    /**/
#define TRAILING_EXPRESSION_COMMENT(value) \
    (value + 1)                            \
    // expression marker

// Empty spliced lines do not determine a raw replacement's common indentation.
#define RAW_BODY(suffix) \
    TEST(Body##suffix) { \
        ns::Value##suffix value; \
\
        Use(value); \
    }

// Separate preprocessing angle tokens must not become a shift token after wrapping.
void RegisterTokens() {
FORMAT_TOKEN_REGISTER(1, values, .template Serializer<TMapSerializer<TTupleSerializer<TCookieAndPool, 2>, TDefaultSerializer, TUnsortedTag > >());
FORMAT_TOKEN_REGISTER(1, values, .template Serializer<TMapSerializer<TTupleSerializer<TCookieAndPool, 2>, TDefaultSerializer, TUnsortedTag>>());
}

// A generated function header can have an ordinary function try block.
DEFINE_FUNCTION(ParseValue, TOptional<TValue>)
try {return Parse();}
catch(const Error& error) {Report(error);return {};}
catch(...) {return {};}

// A macro can provide the handlers required immediately after a try body.
void MacroHandlers() {
try {Read();} CATCH_AND_REPORT("read failed");
Continue();
if(ready) try {Save();} CATCH_AND_REPORT("save failed");
Finish();
try {Read();} catch(...) {Report();}
OrdinaryCallAfterCatch();
}
struct GuardedConstruction {
GuardedConstruction() try : value_(Read()) {} CATCH_AND_REPORT("construction failed");
};

// Class boundary macros expose ordinary members, including nested class scopes.
FORMAT_CLASS_BEGIN(Scalar, int) // generated class header
public:
int Read() const {return value_;}
protected:
int value_=0;
FORMAT_CLASS_END(Scalar, int) // generated class terminator
FORMAT_CLASS_BEGIN_LITERAL
public:
void Run(){Work();}
};
namespace MacroClasses {
template<class T>
FORMAT_CLASS_BEGIN_TEMPLATE(Wrapper, T)
public:
FORMAT_CLASS_BEGIN(Nested,T)
private:
T value_;
FORMAT_CLASS_END(Nested,T);
#if ENABLED
T Read() const {return value_;}
#else
void Read() {}
#endif
FORMAT_CLASS_END_TEMPLATE
void Use() {
FORMAT_CLASS_BEGIN(Local,int)
public:
int Read() const {return 1;}
};
Local value;Consume(value);
}
}
FORMAT_CLASS_BEGIN(WrapperWithADeliberatelyLongGeneratedName, NamespaceWithADeliberatelyLongName::TypeWithADeliberatelyLongName)
public:
int Read() const {return 1;}
FORMAT_CLASS_END(WrapperWithADeliberatelyLongGeneratedName)
FORMAT_CLASS_BEGIN_EMPTY(Empty) FORMAT_CLASS_END_EMPTY(Empty)
// Scope categories also apply inside structured macro replacements.
#define FORMAT_GENERATED_CLASS(name) FORMAT_CLASS_BEGIN(name,int) public: int Read() const {return 1;} FORMAT_CLASS_END(name)
