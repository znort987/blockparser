
// SHA256 test

#include <test.h>
#include <util.h>
#include <sha256.h>
#include <stdint.h>
#include <string.h>

static bool check(
    bool          ok,
    const uint8_t *shaHex,
    const uint8_t *dataHex
) {
    uint8_t sha[kSHA256ByteSize];
    fromHex(sha, shaHex, kSHA256ByteSize, false);

    uint8_t data[1024];
    auto dataLen = strlen((const char*)dataHex)/2;
    if(0<dataLen) {
        fromHex(data, dataHex, dataLen, false);
    }

    uint8_t cmp[kSHA256ByteSize];
    sha256(cmp, data, dataLen);

    TEST_CHECK(
        ok,
        0==memcmp(cmp, sha, kSHA256ByteSize),
        "SHA256 fails for %s\n",
        dataHex
    );
    return ok;
}

static int test() {

    auto ok = true;

    #define T(x, y) {                       \
        const uint8_t shaHex[] = x;         \
        const uint8_t dataHex[] = y;        \
        ok = check(ok, shaHex, dataHex);    \
    }                                       \

        T("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "");
        T("4d7b3ef7300acf70c892d8327db8272f54434adbc61a4e130a563cb59a0d0f47", "0e");
        T("4d4be38d00ef78211228b89d2124736f6131de6b048dd0a9c538f4058bec9c70", "f966");
        T("d5bf2615855abd77089d2e5560a5089e2b31c09b7de5a9eddb78956eb1bbc61c", "be5c42");
        T("16ff538d4e3bd817027081e245b432e52e3caabb8d8bd99ad38cf8e7e8f5b4c7", "2d130498");
        T("0246dcdfc341cde5ad9930429b4a13bfe187ca925d0753540700d8bbc2ab2f75", "c42f7c16cd");
        T("90065e6648c3d0dd50ecbc3586ac9aa3400f787313ef47950e94249e2d334224", "ebe79b8b56bf");
        T("44c988507dfaf704f8025ba679105865e98bc96918a355afe4b6efc1355f6b4e", "f6b26c578ab0b6");
        T("ee69d147375a6ae97e5e333b0f4b79551a969ed4dd95b65d43f978135c718c6c", "c2184f8d67a3e2dc");
        T("5b71d9d29d9bba89a1ca148b9e09df396ba77473d08fd33c767c70e7b360e90a", "3abcf1e14bd42fa6d8");
        T("1ce24a3a822c091a08209fb9dfe7c81e67806a5aa4be3f7cdd9433c75bac0fcf", "d3f4d01786c10aa899c6");
        T("a5529f34273ff90f81b44c4e353c526cce630c990a9b1c2d84de809fbfe7ded8", "7332c66e3293c170e305bd");
        T("a5d1d121bbaaa4f6d670ca636ad2cebf25ff4ddcacb1d73b8973bcc4cce65a58", "1e41625b73ec3e99cd43fd24");
        T("efa296d2b2e63487aced3fdf3334233061411fe46b3225ffe60b00b805fdcea4", "ce3fa4853469501a999d1f1203");
        T("e399295a3611ee78f66a9f1dab5ab4c849acd07a2fe85811f9889b7ff2962624", "b91720e919aac502b5366a0fb883");
        T("e4e69ff71b17590eacd1d62b2b03425c014e7dc4f0041f28fed82b55ac231669", "da842b9ad346beb8f4af9c7d403ec5");
        T("1db741486e0f656255c61ec233a540d72c0d1595f809e16a518ffa7ed676716b", "05b389e8b15acaf46fc20c36a1e3fe89");
        T("d0459f29249b405309d5950980b94361382fe13b3959bfeae2aa066c6dd77c50", "c2cc17a191db9994654e172d66460f2b5d");
        T("9c200941f81b279bff9f3a413717681000b5b6e3870617cbb7e66377ffa50a7a", "92689f59ecc93dffb0eb3ee201bc4374977d");
        T("77fd7853536818c9730f4270de9ea716e11229da22d70aafae8f9147062c12e8", "e643d55870e0141507a14cb576f51483b12b99");
        T("27cda779aa2efee56673cf36e63e7f69bf16e019023bc7ef6c558215a139513a", "e4292cbde5c7c01084ff6fac4bfb63cb76af367b");
        T("3f8db6368020e96fdff9496adee36e20bc172270f7bcc28722789745853f0931", "43756f1b89d58cfe4279dfaa32ccfb9157f37c89d0");
        T("79d0cdc8db6f5143c4eae6eb32c809f7084ffb7e874b33f603612b50167eb8ca", "b4d81766f5bd80fb67e45a5fbb85fdc4692dcab0e643");
        T("7c6e05d4891e03bd8d96d7c24d95a2fab9d69840d628c62bfe3ca49d0868a9c6", "0b71f73711cebadd9aa14ccce53fd42ccd093b98310daf");
        T("8fbb92193a2eaa1f0e21898d3f95ee88a879bf82f485ffe2b1648c4008c6148c", "77668db42eaa158970e190d312f4de7fd0bd32d7a8bfcd5a");
        T("911fabf5838cf6e0fc28021db54e16a07d94b126f3c956f1ad90282191d753a3", "f8fd424a5113b92fa45234b205b35fed71d621372e560b5ebc");
        T("4654f4faed72dae288374466e2d175b7dffc114e5dff90615a47275cf28c1bf4", "42ebf0ed42e41a1e91fc65466b016510d5a4d96a8bc2fff9c282");
        T("47aac0640be1bd88c41bd681aa5815a8c18bfbde117622523357ef4aca63ca58", "7e7a1775fe110bca0220b2944b51515a20682d482cc473cf566d3d");
        T("f4508feddc9295b9efc97f5e7f71c112768b0c04a301de29aa527af16161f44f", "66acedbdf3870eb300357a2f2476d4cc2a8839d32b5f2abefb20aade");
        T("65f36cf9336da79dbdce025e36793377c6e2e319674592ed9ee1788beb0f7398", "e03eac05a028a9492a10e57c478b0d1a9c49cef757cb60ab8ee6202976");

    #undef T

    return ok ? 0 : 1;
}

static SimpleTest t(test,  "sha256");

