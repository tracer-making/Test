#pragma once
#include <cstdint>

// 全局文脉存储：记录战斗对敌人本体造成的溢出伤害总量
class WenMaiStore {
public:
    static WenMaiStore& instance() {
        static WenMaiStore s;
        return s;
    }

    inline void clear() { value_ = 0; }
    inline int get() const { return value_; }
    inline void add(int v) { if (v > 0) value_ += v; }
    inline bool spend(int v) {
        if (v <= 0) return true;
        if (value_ >= v) { value_ -= v; return true; }
        return false;
    }

private:
    WenMaiStore() = default;
    int value_ = 0;
};


