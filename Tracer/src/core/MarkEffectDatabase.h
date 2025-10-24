#pragma once

#include <string>
#include <map>

// 印记效果描述数据库
class MarkEffectDatabase {
public:
    static MarkEffectDatabase& instance() {
        static MarkEffectDatabase s;
        return s;
    }
    
    // 获取印记效果描述
    std::string getMarkDescription(const std::string& mark) const;
    
private:
    std::map<std::string, std::string> markDescriptions_;
    
    MarkEffectDatabase();
};
