#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// 1. GRAPHICS & INPUT INTERFACES (Platform Agnostic)
// ============================================================================

struct Color565 {
    uint16_t value;

    static constexpr Color565 Black()   { return {0x0000}; }
    static constexpr Color565 White()   { return {0xFFFF}; }
    static constexpr Color565 Red()     { return {0xF800}; }
    static constexpr Color565 Green()   { return {0x07E0}; }
    static constexpr Color565 Blue()    { return {0x001F}; }
    static constexpr Color565 Yellow()  { return {0xFFE0}; }
    static constexpr Color565 Purple()  { return {0x780F}; }
    static constexpr Color565 FromRGB(uint8_t r, uint8_t g, uint8_t b) {
        return { static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)) };
    }
};

struct InputState {
    bool up       : 1;
    bool down     : 1;
    bool left     : 1;
    bool right    : 1;
    bool confirm  : 1;
    bool cancel   : 1;
    
    bool confirmPressed : 1;
    bool cancelPressed  : 1;
    bool upPressed      : 1;
    bool downPressed    : 1;
    bool leftPressed    : 1;
    bool rightPressed   : 1;
};

class ICanvas {
public:
    virtual ~ICanvas() = default;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual void DrawPixel(int16_t x, int16_t y, Color565 color) = 0;
    virtual void Clear(Color565 color) = 0;

    void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color565 color) {
        for (int16_t py = y; py < y + h; ++py) {
            for (int16_t px = x; px < x + w; ++px) {
                DrawPixel(px, py, color);
            }
        }
    }
};

// ============================================================================
// 2. ENUMS & STAT DEFINITIONS
// ============================================================================

enum class Race : uint8_t { DWARF, ELF, HUMAN, HALFLING };
enum class CharacterClass : uint8_t { FIGHTER, MAGE, CLERIC, ROGUE };

enum class RollMode : uint8_t { NORMAL, ADVANTAGE, DISADVANTAGE };

enum class EnchantmentType : uint8_t {
    NONE,
    FLAT_STR_BONUS,
    FLAT_DEX_BONUS,
    FLAT_INT_BONUS,
    ADD_D4_TO_HIT,
    ADD_D4_TO_DAMAGE
};

struct ItemEnchantment {
    EnchantmentType type;
    int8_t magnitude;
};

enum class ItemType : uint8_t { WEAPON, ACCESSORY };

struct Item {
    const char* name;
    ItemType type;
    int8_t baseDamageDice; // e.g., 6 for d6 damage
    int8_t flatBonus;
    ItemEnchantment enchantment;
};

struct Attributes {
    int16_t hp;
    int16_t maxHp;
    int16_t strength;
    int16_t dexterity;
    int16_t intelligence;
    int16_t armorClass;
};

struct PlayerCharacter {
    const char* name;
    Race race;
    CharacterClass charClass;
    Attributes stats;
    bool isAlive;
    Item inventory[2]; // Slot 0: Weapon, Slot 1: Accessory
};

struct Enemy {
    const char* name;
    Attributes stats;
    int8_t damageDice;
    bool isAlive;
};

enum class RoomType : uint8_t { EMPTY, REGULAR_COMBAT, TREASURE, BOSS_COMBAT, REST_SITE };
enum class GameMode : uint8_t { DUNGEON_MAP, SUB_LOOP_COMBAT, SUB_LOOP_TREASURE, GAME_OVER, VICTORY };

// ============================================================================
// 3. CORE ENGINE CLASS
// ============================================================================

class GameEngine {
public:
    static constexpr int16_t VIRTUAL_WIDTH = 240;
    static constexpr int16_t VIRTUAL_HEIGHT = 240;
    
    static constexpr uint8_t MAX_PARTY_SIZE = 4;
    static constexpr uint8_t MAX_ENEMIES = 3;
    static constexpr uint8_t MAX_ROOMS = 10;

    void Init(uint32_t randomSeed, uint8_t partySize) {
        m_seed = randomSeed;
        m_currentMode = GameMode::DUNGEON_MAP;
        
        m_partySize = (partySize > MAX_PARTY_SIZE) ? MAX_PARTY_SIZE : ((partySize == 0) ? 1 : partySize);
        m_currentRoomIndex = 0;
        m_selectedPartyMember = 0;
        m_selectedTarget = 0;

        InitializeParty();
        GenerateDungeon();
    }

    void Update(const InputState& input, float dt) {
        // Check overall party wipe
        bool partyAlive = false;
        for (uint8_t i = 0; i < m_partySize; ++i) {
            if (m_party[i].isAlive) partyAlive = true;
        }
        if (!partyAlive && m_currentMode != GameMode::GAME_OVER) {
            m_currentMode = GameMode::GAME_OVER;
            return;
        }

        switch (m_currentMode) {
            case GameMode::DUNGEON_MAP:
                UpdateMapLoop(input);
                break;
            case GameMode::SUB_LOOP_COMBAT:
                UpdateCombatLoop(input);
                break;
            case GameMode::GAME_OVER:
            case GameMode::VICTORY:
                if (input.confirmPressed) Init(m_seed + 1, m_partySize);
                break;
            default:
                break;
        }
    }

    void Render(ICanvas& canvas) {
        canvas.Clear(Color565::Black());

        switch (m_currentMode) {
            case GameMode::DUNGEON_MAP:
                RenderMap(canvas);
                break;
            case GameMode::SUB_LOOP_COMBAT:
                RenderCombat(canvas);
                break;
            case GameMode::GAME_OVER:
                canvas.Clear(Color565::Red());
                break;
            case GameMode::VICTORY:
                canvas.Clear(Color565::Green());
                break;
            default:
                break;
        }
    }

protected:
    // Calculate effective total attribute including inventory bonuses
    int16_t GetEffectiveStat(const PlayerCharacter& p, uint8_t statType) {
        int16_t val = 0;
        if (statType == 0) val = p.stats.strength;
        else if (statType == 1) val = p.stats.dexterity;
        else if (statType == 2) val = p.stats.intelligence;

        for (uint8_t i = 0; i < 2; ++i) {
            const ItemEnchantment& ench = p.inventory[i].enchantment;
            if (statType == 0 && ench.type == EnchantmentType::FLAT_STR_BONUS) val += ench.magnitude;
            if (statType == 1 && ench.type == EnchantmentType::FLAT_DEX_BONUS) val += ench.magnitude;
            if (statType == 2 && ench.type == EnchantmentType::FLAT_INT_BONUS) val += ench.magnitude;
        }
        return val;
    }

private:
    uint32_t m_seed;
    uint32_t Random() {
        m_seed = m_seed * 1664525UL + 1013904223UL;
        return m_seed;
    }
    
    int32_t RandomRange(int32_t min, int32_t max) {
        return min + (static_cast<int32_t>(Random() % static_cast<uint32_t>(max - min + 1)));
    }

    // Dice Rolling Engine
    int16_t RollD4() { return static_cast<int16_t>(RandomRange(1, 4)); }
    int16_t RollD6() { return static_cast<int16_t>(RandomRange(1, 6)); }

    int16_t RollD20(RollMode mode = RollMode::NORMAL) {
        int16_t r1 = static_cast<int16_t>(RandomRange(1, 20));
        if (mode == RollMode::NORMAL) return r1;
        
        int16_t r2 = static_cast<int16_t>(RandomRange(1, 20));
        if (mode == RollMode::ADVANTAGE) return (r1 > r2) ? r1 : r2;
        return (r1 < r2) ? r1 : r2; // Disadvantage
    }

    // Engine Core State
    GameMode m_currentMode;
    PlayerCharacter m_party[MAX_PARTY_SIZE];
    uint8_t m_partySize;
    
    Enemy m_enemies[MAX_ENEMIES];
    uint8_t m_enemyCount;

    uint8_t m_currentRoomIndex;
    uint8_t m_selectedPartyMember; // Chosen hero for special ability
    uint8_t m_selectedTarget;     // Primary target index

    struct RoomNode {
        uint8_t id;
        RoomType type;
        bool cleared;
    } m_dungeon[MAX_ROOMS];

    // ========================================================================
    // INITIALIZATION & GENERATION
    // ========================================================================
    void InitializeParty() {
        const char* names[] = {"Thorin", "Lyra", "Eldrin", "Finn"};
        Race races[] = {Race::DWARF, Race::ELF, Race::HUMAN, Race::HALFLING};
        CharacterClass classes[] = {CharacterClass::FIGHTER, CharacterClass::MAGE, CharacterClass::CLERIC, CharacterClass::ROGUE};

        for (uint8_t i = 0; i < m_partySize; ++i) {
            PlayerCharacter& p = m_party[i];
            p.name = names[i];
            p.race = races[i % 4];
            p.charClass = classes[i % 4];
            p.isAlive = true;

            // Base Stats derived from Race
            int16_t baseHp = 25, baseStr = 10, baseDex = 10, baseInt = 10;
            switch (p.race) {
                case Race::DWARF:    baseHp += 10; baseStr += 3; break;
                case Race::ELF:      baseInt += 4; baseDex += 2; break;
                case Race::HUMAN:    baseHp += 4; baseStr += 1; baseDex += 1; baseInt += 1; break;
                case Race::HALFLING: baseDex += 4; baseHp += 2; break;
            }

            // Stat Variance (+/- 2 points)
            p.stats.maxHp = baseHp + static_cast<int16_t>(RandomRange(-2, 4));
            p.stats.hp = p.stats.maxHp;
            p.stats.strength = baseStr + static_cast<int16_t>(RandomRange(-2, 2));
            p.stats.dexterity = baseDex + static_cast<int16_t>(RandomRange(-2, 2));
            p.stats.intelligence = baseInt + static_cast<int16_t>(RandomRange(-2, 2));
            p.stats.armorClass = 10 + (p.stats.dexterity - 10) / 2;

            // Default Inventory Assignment
            p.inventory[0] = {"Shortsword", ItemType::WEAPON, 6, 0, {EnchantmentType::NONE, 0}};
            p.inventory[1] = {"Iron Ring", ItemType::ACCESSORY, 0, 0, {EnchantmentType::FLAT_STR_BONUS, 1}};
        }
    }

    void GenerateDungeon() {
        for (uint8_t i = 0; i < MAX_ROOMS; ++i) {
            m_dungeon[i].id = i;
            m_dungeon[i].cleared = false;
            m_dungeon[i].type = (i == MAX_ROOMS - 1) ? RoomType::BOSS_COMBAT : RoomType::REGULAR_COMBAT;
        }
    }

    // ========================================================================
    // COMBAT ENGINE (Turn Execution & Mechanics)
    // ========================================================================
    void SetupCombat(bool isBoss) {
        m_enemyCount = isBoss ? 1 : static_cast<uint8_t>(RandomRange(1, MAX_ENEMIES));
        
        for (uint8_t i = 0; i < m_enemyCount; ++i) {
            Enemy& e = m_enemies[i];
            e.isAlive = true;
            e.name = isBoss ? "Dungeon Overlord" : "Orc Raider";
            
            int16_t hpBase = isBoss ? 80 : 20;
            int16_t variance = static_cast<int16_t>(RandomRange(-3, 5));
            e.stats.maxHp = hpBase + variance;
            e.stats.hp = e.stats.maxHp;
            e.stats.armorClass = isBoss ? 14 : 11;
            e.stats.strength = 12 + static_cast<int16_t>(RandomRange(-1, 3));
            e.damageDice = isBoss ? 8 : 6;
        }
        
        m_selectedPartyMember = 0;
        m_selectedTarget = 0;
    }

    void UpdateMapLoop(const InputState& input) {
        if (input.confirmPressed) {
            SetupCombat(m_dungeon[m_currentRoomIndex].type == RoomType::BOSS_COMBAT);
            m_currentMode = GameMode::SUB_LOOP_COMBAT;
        }
    }

    void UpdateCombatLoop(const InputState& input) {
        // Selection Cycle: Left/Right selects Special Hero, Up/Down selects Target Enemy
        if (input.leftPressed) {
            do {
                m_selectedPartyMember = (m_selectedPartyMember + m_partySize - 1) % m_partySize;
            } while (!m_party[m_selectedPartyMember].isAlive);
        }
        if (input.rightPressed) {
            do {
                m_selectedPartyMember = (m_selectedPartyMember + 1) % m_partySize;
            } while (!m_party[m_selectedPartyMember].isAlive);
        }
        if (input.downPressed || input.upPressed) {
            do {
                m_selectedTarget = (m_selectedTarget + 1) % m_enemyCount;
            } while (!m_enemies[m_selectedTarget].isAlive);
        }

        // Confirm Phase: Resolve Player Turn then Enemy Retaliation
        if (input.confirmPressed) {
            ExecutePartyTurn();

            // Check if all enemies defeated
            bool enemiesDefeated = true;
            for (uint8_t i = 0; i < m_enemyCount; ++i) {
                if (m_enemies[i].isAlive) enemiesDefeated = false;
            }

            if (enemiesDefeated) {
                m_dungeon[m_currentRoomIndex].cleared = true;
                m_currentRoomIndex++;
                if (m_currentRoomIndex >= MAX_ROOMS) {
                    m_currentMode = GameMode::VICTORY;
                } else {
                    m_currentMode = GameMode::DUNGEON_MAP;
                }
                return;
            }

            // Enemy Attack Cycle
            ExecuteEnemyTurn();
        }
    }

    void ExecutePartyTurn() {
        // 1. Chosen Hero executes Class Special Action
        PlayerCharacter& specialHero = m_party[m_selectedPartyMember];
        ExecuteClassSpecial(specialHero, m_selectedTarget);

        // 2. Remaining living heroes execute basic weapon attacks
        for (uint8_t i = 0; i < m_partySize; ++i) {
            if (i == m_selectedPartyMember || !m_party[i].isAlive) continue;
            
            // Find valid enemy target
            uint8_t targetIdx = m_selectedTarget;
            if (!m_enemies[targetIdx].isAlive) {
                for (uint8_t e = 0; e < m_enemyCount; ++e) {
                    if (m_enemies[e].isAlive) { targetIdx = e; break; }
                }
            }
            
            ExecuteBasicAttack(m_party[i], m_enemies[targetIdx]);
        }
    }

    void ExecuteBasicAttack(PlayerCharacter& attacker, Enemy& target) {
        if (!target.isAlive) return;

        int16_t strMod = (GetEffectiveStat(attacker, 0) - 10) / 2;
        int16_t attackRoll = RollD20(RollMode::NORMAL) + strMod;

        // Apply weapon enchantment to-hit bonuses
        if (attacker.inventory[0].enchantment.type == EnchantmentType::ADD_D4_TO_HIT) {
            attackRoll += RollD4();
        }

        // Hit Resolution against Target AC
        if (attackRoll >= target.stats.armorClass) {
            int16_t damage = static_cast<int16_t>(RandomRange(1, attacker.inventory[0].baseDamageDice)) + strMod;
            if (attacker.inventory[0].enchantment.type == EnchantmentType::ADD_D4_TO_DAMAGE) {
                damage += RollD4();
            }
            if (damage < 1) damage = 1;

            target.stats.hp -= damage;
            if (target.stats.hp <= 0) {
                target.stats.hp = 0;
                target.isAlive = false;
            }
        }
    }

    void ExecuteClassSpecial(PlayerCharacter& hero, uint8_t targetIdx) {
        Enemy& target = m_enemies[targetIdx];

        switch (hero.charClass) {
            case CharacterClass::FIGHTER: {
                // Advantage on Hit Roll + Modified Weapon Damage
                int16_t strMod = (GetEffectiveStat(hero, 0) - 10) / 2;
                if (RollD20(RollMode::ADVANTAGE) + strMod >= target.stats.armorClass) {
                    int16_t damage = static_cast<int16_t>(RandomRange(1, hero.inventory[0].baseDamageDice)) + strMod + 4;
                    target.stats.hp -= damage;
                    if (target.stats.hp <= 0) { target.stats.hp = 0; target.isAlive = false; }
                }
                break;
            }
            case CharacterClass::MAGE: {
                // Guaranteed Magic Missile Area Damage (Ignores AC)
                int16_t intMod = (GetEffectiveStat(hero, 2) - 10) / 2;
                for (uint8_t e = 0; e < m_enemyCount; ++e) {
                    if (m_enemies[e].isAlive) {
                        m_enemies[e].stats.hp -= (RollD4() + 1 + intMod);
                        if (m_enemies[e].stats.hp <= 0) { m_enemies[e].stats.hp = 0; m_enemies[e].isAlive = false; }
                    }
                }
                break;
            }
            case CharacterClass::CLERIC: {
                // Heals lowest HP party member
                uint8_t lowestIdx = 0;
                for (uint8_t i = 1; i < m_partySize; ++i) {
                    if (m_party[i].isAlive && m_party[i].stats.hp < m_party[lowestIdx].stats.hp) {
                        lowestIdx = i;
                    }
                }
                int16_t healAmount = RollD4() + 4 + (GetEffectiveStat(hero, 2) - 10) / 2;
                m_party[lowestIdx].stats.hp += healAmount;
                if (m_party[lowestIdx].stats.hp > m_party[lowestIdx].stats.maxHp) {
                    m_party[lowestIdx].stats.hp = m_party[lowestIdx].stats.maxHp;
                }
                break;
            }
            case CharacterClass::ROGUE: {
                // Backstab: Auto-critical on hit
                int16_t dexMod = (GetEffectiveStat(hero, 1) - 10) / 2;
                if (RollD20(RollMode::NORMAL) + dexMod >= target.stats.armorClass) {
                    int16_t damage = (static_cast<int16_t>(RandomRange(1, hero.inventory[0].baseDamageDice)) + dexMod) * 2;
                    target.stats.hp -= damage;
                    if (target.stats.hp <= 0) { target.stats.hp = 0; target.isAlive = false; }
                }
                break;
            }
        }
    }

    void ExecuteEnemyTurn() {
        for (uint8_t e = 0; e < m_enemyCount; ++e) {
            if (!m_enemies[e].isAlive) continue;

            // Target random living party member
            uint8_t targetIdx = static_cast<uint8_t>(RandomRange(0, m_partySize - 1));
            while (!m_party[targetIdx].isAlive) {
                targetIdx = (targetIdx + 1) % m_partySize;
            }

            PlayerCharacter& target = m_party[targetIdx];
            int16_t attackRoll = RollD20(RollMode::NORMAL) + (m_enemies[e].stats.strength - 10) / 2;

            if (attackRoll >= target.stats.armorClass) {
                int16_t dmg = static_cast<int16_t>(RandomRange(1, m_enemies[e].damageDice));
                target.stats.hp -= dmg;
                if (target.stats.hp <= 0) {
                    target.stats.hp = 0;
                    target.isAlive = false;
                }
            }
        }
    }

    // ========================================================================
    // UI RENDERING
    // ========================================================================
    void RenderMap(ICanvas& canvas) {
        canvas.DrawRect(10, 10, 220, 30, Color565::FromRGB(40, 40, 40));
    }

    void RenderCombat(ICanvas& canvas) {
        // Render Party (Left side)
        for (uint8_t i = 0; i < m_partySize; ++i) {
            if (m_party[i].isAlive) {
                Color565 color = (i == m_selectedPartyMember) ? Color565::Yellow() : Color565::Blue();
                canvas.DrawRect(10, 20 + (i * 50), 70, 40, color);
            }
        }

        // Render Enemies (Right side)
        for (uint8_t i = 0; i < m_enemyCount; ++i) {
            if (m_enemies[i].isAlive) {
                Color565 color = (i == m_selectedTarget) ? Color565::Yellow() : Color565::Red();
                canvas.DrawRect(150, 30 + (i * 60), 60, 45, color);
            }
        }
    }
};