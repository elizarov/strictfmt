// Two-, three-, and four-byte characters.
void CharacterWidths() {
    Prepare();
    Use("Доставка недоступна");
    Use(U'𝄞', u8"Доставка", L"é");
    Use("012345678901234567890123456é");
    Use("012345678901234567890123456€");
    Use("012345678901234567890123456𝄞");
    Use(
        "012345678901234567890123456é!"
    );
    Use(
        "012345678901234567890123456€!"
    );
    Use(
        "012345678901234567890123456𝄞!"
    );
}

// Combining accents have the same width.
void CombiningCharacters() {
    Prepare();
    Use("012345678901234567890123456é");
    Use(
        "012345678901234567890123456é!"
    );
    Use("012345678901234567890123456á̧");
    Use(
        "012345678901234567890123456á̧!"
    );
}

// Emoji sequences remain one character.
void EmojiClusters() {
    Prepare();
    Use("012345678901234567890123456👩🏽‍💻");
    Use(
        "012345678901234567890123456👩🏽‍💻!"
    );
    Use("012345678901234567890123456🇫🇷");
    Use(
        "012345678901234567890123456🇫🇷!"
    );
    Use("012345678901234567890123456♥️");
    Use(
        "012345678901234567890123456♥️!"
    );
    Use("0123456789012345678901234561️⃣");
    Use(
        "0123456789012345678901234561️⃣!"
    );
}

// Conjoining letters and spacing marks.
void OtherClusters() {
    Prepare();
    Use("012345678901234567890123456각");
    Use(
        "012345678901234567890123456각!"
    );
    Use("012345678901234567890123456क्षि");
    Use(
        "012345678901234567890123456क्षि!"
    );
}

// Unicode names use the same counter.
namespace данные {

struct Запись {
    int количество;
};

void Обработать(Запись запись);

struct Источник {
    operator Запись() const;
};

}
void Identifiers() {
    Prepare();
    ДоставкаНедоступнаДляАдреса(
        значение
    );
    处理订单并发送通知(收件人, 订单号);
    𐐀𐐁𐐂𐐃𐐄𐐅𐐆𐐇𐐈𐐉𐐊𐐋𐐌𐐍𐐎𐐏𐐐𐐑𐐒𐐓(𐐀𐐁𐐂𐐃);
    cafécafécafécafécafécafé(café);
    данные::Обработать(
        данные::Запись{42}
    );
}

// A joined spelling is measured anew.
void LiteralJoining() {
    Prepare();
    Use("012345678901234567890123456é");
    Use("012345678901234567890123456é");
    Use("012345678901234567890123456👩🏽‍💻");
    Use(
        "012345678901234567890123456𝄞!"
    );
    Use(
        "012345678901234567890123456\u00e9"
    );
}

// Widths compose through both chains.
void LiteralPairs() {
    auto text = first +
        "сообщение=" + value +
        "𝄞=" + other;
    stream
        << "сообщение=" << value
        << "é=" << other;
}

// Comment alignment uses character width.
struct CommentWidths {
    int я;       // Доставка скоро
    int longer;  // 𝄞 é 👩🏽‍💻

    int é;    // Скоро
    int имя;  // Доставка

    int a;  // 01234567890123456789012345é
    int longer;  // outside the limit
};

void CommentWrapping() {
    Prepare();
    Use("доставка", /* é 𝄞 👩🏽‍💻 */ value);
    Use(
        "доставка",  // é 𝄞 👩🏽‍💻
        value
    );
    auto result = first +  // сообщение
        second +
        third;
}

// Each physical raw-string line is priced.
void MultilineStrings() {
    Prepare();
    Use(R"(Доставка недоступна
01234567890123456789012345678901234é)");
    Use(
        R"(Доставка недоступна
01234567890123456789012345678901234é)",
        x
    );
    LongerUse(
        R"(0123456789012345678901234567890𝄞
Доставка)"
    );
}

// Macro continuations include their suffix.
#define СООБЩЕНИЕ \
    Use("01234567890123456789é𝄞")
#if ДОСТАВКА  // сообщение é
int значение;
#endif  // 𝄞 👩🏽‍💻
