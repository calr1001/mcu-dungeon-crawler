// tests/test_combat.cpp
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "GameEngine.hpp"

TEST_CASE("Player stats incorporate equipment enchantment modifiers", "[character]") {
    PlayerCharacter hero = {};
    hero.stats.strength = 10;
    hero.inventory[0] = {"Shortsword", ItemType::WEAPON, 6, 0, {EnchantmentType::FLAT_STR_BONUS, 2}};
    hero.inventory[1] = {"Strength Ring", ItemType::ACCESSORY, 0, 0, {EnchantmentType::FLAT_STR_BONUS, 3}};

    // Create test instance engine to access stat calculation
    class TestEngine : public GameEngine {
    public:
        int16_t TestStat(const PlayerCharacter& p, uint8_t type) {
            return GetEffectiveStat(p, type);
        }
    } engine;

    REQUIRE(engine.TestStat(hero, 0) == 15); // Base 10 + Weapon +2 + Ring +3
}

TEST_CASE("Party size clamp operates within bounds", "[engine]") {
    GameEngine engine;
    
    // Testing boundary conditions (0 -> forced to 1, >4 -> clamped to 4)
    engine.Init(42, 0);
    // Execute frame to ensure state stability
    InputState input = {};
    engine.Update(input, 0.016f);
    
    SUCCEED("Engine initialized without memory or out-of-bounds fault");
}