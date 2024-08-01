#include "cpp-common/bt2/component-class-dev.hpp"
#include "cpp-common/bt2/graph.hpp"
#include "cpp-common/bt2/plugin-load.hpp"

class MaSource;

class MaIterateur : public bt2::UserMessageIterator<MaIterateur, MaSource>
{
public:
    explicit MaIterateur(const bt2::SelfMessageIterator self, bt2::SelfMessageIteratorConfiguration,
                         bt2::SelfComponentOutputPort) :
        bt2::UserMessageIterator<MaIterateur, MaSource> {self, "MA-ITER"}
    {
    }
    void _next(bt2::ConstMessageArray& msgs);

private:
    bool _mDone = false;

    static void _getSupportedMipVersions(bt2::SelfComponentClass, bt2::ConstValue,
                                         bt2::LoggingLevel, bt2::UnsignedIntegerRangeSet ranges)
    {
        ranges.addRange(0, 1);
    }
};

class MaSource : public bt2::UserSourceComponent<MaSource, MaIterateur>
{
    friend class MaIterateur;

public:
    static constexpr auto name = "ma-source";
    explicit MaSource(const bt2::SelfSourceComponent self, bt2::ConstMapValue, void *) :
        bt2::UserSourceComponent<MaSource, MaIterateur> {self, "MA-SRC"}
    {
        this->_addOutputPort("out");
    }
    static void _getSupportedMipVersions(bt2::SelfComponentClass, bt2::ConstValue,
                                         bt2::LoggingLevel, bt2::UnsignedIntegerRangeSet ranges)
    {
        ranges.addRange(1, 1);
    }
};

void MaIterateur::_next(bt2::ConstMessageArray& msgs)
{
    if (_mDone) {
        return;
    }
    const auto traceCls = this->_selfComponent().createTraceClass();
    const auto trace = traceCls->instantiate();
    trace->nameSpace("le-namespace").name("le-name").uid("le-uid");
    const auto streamCls = traceCls->createStreamClass();
    streamCls->nameSpace("le-namespace")
        .name("le-name")
        .uid("le-uid")
        .defaultClockClass(this->_selfComponent()
                               .createClockClass()
                               ->origin("le-namespace", "le-name", "le-uid")
                               .nameSpace("le-namespace")
                               .name("le-name")
                               .uid("le-uid")
                               .precision(12)
                               .accuracy(24));
    const auto stream = streamCls->instantiate(*trace);
    stream->name("le-name");
    msgs.append(this->_createStreamBeginningMessage(*stream));
    msgs.append(bt2c::call([&] {
        const auto eventMsg = this->_createEventMessage(
            streamCls->createEventClass()
                ->nameSpace("le-namespace")
                .name("le-name")
                .uid("le-uid")
                .payloadFieldClass(
                    (*traceCls->createStructureFieldClass())
                        .appendMember("static-blob",
                                      traceCls->createStaticBlobFieldClass(1)->mediaType(
                                          "application/vnd.rar"))
                        .appendMember(
                            "dynamic-blob-without-field-location",
                            *traceCls->createDynamicBlobWithoutLengthFieldLocationFieldClass())
                        .appendMember("dynamic-blob-with-length-field-location-length",
                                      *traceCls->createUnsignedIntegerFieldClass())
                        .appendMember(
                            "dynamic-blob-with-length-field-location",
                            *traceCls->createDynamicBlobWithLengthFieldLocationFieldClass(
                                *traceCls->createFieldLocation(
                                    bt2::ConstFieldLocation::Scope::EventPayload,
                                    std::vector<const char *> {
                                        "dynamic-blob-with-length-field-location-length"})))
                        .appendMember("bit-array",
                                      traceCls->createBitArrayFieldClass(64)->addFlag(
                                          "flag-foo",
                                          bt2::UnsignedIntegerRangeSet::create()->addRange(0, 12)))
                        .appendMember(
                            "dynamic-array-without-field-location",
                            *traceCls->createDynamicArrayWithoutLengthFieldLocationFieldClass(
                                *traceCls->createUnsignedIntegerFieldClass()))
                        .appendMember("dynamic-array-with-length-field-location-length",
                                      *traceCls->createUnsignedIntegerFieldClass())
                        .appendMember(
                            "dynamic-array-with-length-field-location",
                            *traceCls->createDynamicArrayWithLengthFieldLocationFieldClass(
                                *traceCls->createUnsignedIntegerFieldClass(),
                                *traceCls->createFieldLocation(
                                    bt2::ConstFieldLocation::Scope::EventPayload,
                                    std::vector<const char *> {
                                        "dynamic-array-with-length-field-location-length"})))
                        .appendMember("option-without-selector-field-location",
                                      *traceCls->createOptionWithoutSelectorFieldLocationFieldClass(
                                          *traceCls->createUnsignedIntegerFieldClass()))
                        .appendMember("option-with-bool-selector-field-location-selector",
                                      *traceCls->createBoolFieldClass())
                        .appendMember(
                            "option-with-bool-selector-field-location",
                            *traceCls->createOptionWithBoolSelectorFieldLocationFieldClass(
                                *traceCls->createUnsignedIntegerFieldClass(),
                                *traceCls->createFieldLocation(
                                    bt2::ConstFieldLocation::Scope::EventPayload,
                                    std::vector<const char *> {
                                        "option-with-bool-selector-field-location-selector"})))
                        .appendMember(
                            "option-with-unsigned-integer-selector-field-location-selector",
                            *traceCls->createUnsignedIntegerFieldClass())
                        .appendMember(
                            "option-with-unsigned-integer-selector-field-location",
                            *traceCls->createOptionWithUnsignedIntegerSelectorFieldLocationFieldClass(
                                *traceCls->createUnsignedIntegerFieldClass(),
                                *traceCls->createFieldLocation(
                                    bt2::ConstFieldLocation::Scope::EventPayload,
                                    std::vector<const char *> {
                                        "option-with-unsigned-integer-selector-field-location-selector"}),
                                bt2::UnsignedIntegerRangeSet::create()->addRange(1, 1)))
                        .appendMember("option-with-signed-integer-selector-field-location-selector",
                                      *traceCls->createSignedIntegerFieldClass())
                        .appendMember(
                            "option-with-signed-integer-selector-field-location",
                            *traceCls->createOptionWithSignedIntegerSelectorFieldLocationFieldClass(
                                *traceCls->createUnsignedIntegerFieldClass(),
                                *traceCls->createFieldLocation(
                                    bt2::ConstFieldLocation::Scope::EventPayload,
                                    std::vector<const char *> {
                                        "option-with-signed-integer-selector-field-location-selector"}),
                                bt2::SignedIntegerRangeSet::create()->addRange(1, 1)))
                        .appendMember(
                            "variant-without-selector-field-location",
                            traceCls->createVariantWithoutSelectorFieldLocationFieldClass()
                                ->appendOption("variant-option",
                                               *traceCls->createUnsignedIntegerFieldClass()))
                        .appendMember(
                            "variant-with-unsigned-integer-selector-field-location-selector",
                            *traceCls->createUnsignedIntegerFieldClass())
                        .appendMember(
                            "variant-with-unsigned-integer-selector-field-location",
                            traceCls
                                ->createVariantWithUnsignedIntegerSelectorFieldLocationFieldClass(
                                    *traceCls->createFieldLocation(
                                        bt2::ConstFieldLocation::Scope::EventPayload,
                                        std::vector<const char *> {
                                            "variant-with-unsigned-integer-selector-field-location-selector"}))
                                ->appendOption(
                                    "variant-option", *traceCls->createUnsignedIntegerFieldClass(),
                                    bt2::UnsignedIntegerRangeSet::create()->addRange(1, 1)))
                        .appendMember(
                            "variant-with-signed-integer-selector-field-location-selector",
                            *traceCls->createSignedIntegerFieldClass())
                        .appendMember(
                            "variant-with-signed-integer-selector-field-location",
                            traceCls
                                ->createVariantWithSignedIntegerSelectorFieldLocationFieldClass(
                                    *traceCls->createFieldLocation(
                                        bt2::ConstFieldLocation::Scope::EventPayload,
                                        std::vector<const char *> {
                                            "variant-with-signed-integer-selector-field-location-selector"}))
                                ->appendOption(
                                    "variant-option", *traceCls->createUnsignedIntegerFieldClass(),
                                    bt2::SignedIntegerRangeSet::create()->addRange(1, 1)))),
            *stream, 123);
        const auto payload = *eventMsg->event().payloadField();
        payload["static-blob"]->asBlob().data()[0] = 0x11;
        payload["dynamic-blob-without-field-location"]->asDynamicBlob().length(1).data()[0] = 0x22;
        payload["dynamic-blob-with-length-field-location-length"]->asUnsignedInteger().value(1);
        payload["dynamic-blob-with-length-field-location"]->asDynamicBlob().length(1).data()[0] =
            0x33;
        payload["bit-array"]->asBitArray().valueAsInteger(0xffffffffffffffff);
        payload["dynamic-array-without-field-location"]
            ->asDynamicArray()
            .length(1)[0]
            .asUnsignedInteger()
            .value(1234);
        payload["dynamic-array-with-length-field-location-length"]->asUnsignedInteger().value(1);
        payload["dynamic-array-with-length-field-location"]
            ->asDynamicArray()
            .length(1)[0]
            .asUnsignedInteger()
            .value(2345);
        payload["option-without-selector-field-location"]
            ->asOption()
            .hasField(true)
            .field()
            ->asUnsignedInteger()
            .value(111);
        payload["option-with-bool-selector-field-location-selector"]->asBool().value(1);
        payload["option-with-bool-selector-field-location"]
            ->asOption()
            .hasField(true)
            .field()
            ->asUnsignedInteger()
            .value(222);
        payload["option-with-unsigned-integer-selector-field-location-selector"]
            ->asUnsignedInteger()
            .value(1);
        payload["option-with-unsigned-integer-selector-field-location"]
            ->asOption()
            .hasField(true)
            .field()
            ->asUnsignedInteger()
            .value(333);
        payload["option-with-signed-integer-selector-field-location-selector"]
            ->asSignedInteger()
            .value(1);
        payload["option-with-signed-integer-selector-field-location"]
            ->asOption()
            .hasField(true)
            .field()
            ->asUnsignedInteger()
            .value(333);
        payload["variant-without-selector-field-location"]
            ->asVariant()
            .selectOption(0)
            .selectedOptionField()
            .asUnsignedInteger()
            .value(444);
        payload["variant-with-unsigned-integer-selector-field-location-selector"]
            ->asUnsignedInteger()
            .value(1);
        payload["variant-with-unsigned-integer-selector-field-location"]
            ->asVariant()
            .selectOption(0)
            .selectedOptionField()
            .asUnsignedInteger()
            .value(555);
        payload["variant-with-signed-integer-selector-field-location-selector"]
            ->asSignedInteger()
            .value(1);
        payload["variant-with-signed-integer-selector-field-location"]
            ->asVariant()
            .selectOption(0)
            .selectedOptionField()
            .asUnsignedInteger()
            .value(666);
        return eventMsg;
    }));
    msgs.append(this->_createStreamEndMessage(*stream));
    _mDone = true;
}

int main()
{
    const auto textPlugin = bt2::findPlugin("text");
    BT_ASSERT(textPlugin);
    const auto detailsCompCls = textPlugin->sinkComponentClasses()["details"];
    BT_ASSERT(detailsCompCls);
    const auto lttngUtilsPlugin = bt2::findPlugin("lttng-utils");
    BT_ASSERT(lttngUtilsPlugin);
    const auto debugInfoCompCls = lttngUtilsPlugin->filterComponentClasses()["debug-info"];
    BT_ASSERT(debugInfoCompCls);
    const auto graph = bt2::Graph::create(1);
    const auto maSourceCompCls = bt2::SourceComponentClass::create<MaSource>();
    const auto src = graph->addComponent(*maSourceCompCls, "la-source");
    const auto snk = graph->addComponent(*detailsCompCls, "le-sink");

    /* For easy flipping between using the filter and not */
    const bool runWithFilter = true;
    if (runWithFilter) {
        const auto flt __attribute__((unused)) = graph->addComponent(*debugInfoCompCls, "le-filtre");

        graph->connectPorts(*src.outputPorts()["out"], *flt.inputPorts()["in"]);
        graph->connectPorts(*flt.outputPorts()["out"], *snk.inputPorts()["in"]);
    } else {
        graph->connectPorts(*src.outputPorts()["out"], *snk.inputPorts()["in"]);
    }

    graph->run();
}
