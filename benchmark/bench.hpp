#include <random>
#ifdef _MSC_VER
    #include <format>
#else // compatability support:
    #define FMT_HEADER_ONLY
    #include <fmt/format.h>
    namespace std { using namespace fmt; }
#endif

constexpr size_t BMEntities = 512; //16 * 1024;

using TimeDelta = double;

struct PositionComponent {
  float x{0.0F};
  float y{0.0F};
};

struct VelocityComponent {
  float x{0.0F};
  float y{0.0F};
};

struct DataComponent {
  inline static constexpr size_t StringyMaxLength = 23;

  int thingy{0};
  double dingy{0.0};
  bool mingy{false};
  /// @FIXME(pico_ecs): SIGSEGV (Segmentation fault), can't copy string ... support for components with dynamic memory
  /// (std::string) ?
  // std::string stringy;
  char stringy[StringyMaxLength + 1] = {0};
};


inline void updatePosition(PositionComponent& position, const VelocityComponent& direction, TimeDelta dt) {
    position.x += direction.x * dt;
    position.y += direction.y * dt;
}

static std::random_device m_rd;
static std::mt19937 m_eng;
inline int random(int min, int max) {
    std::uniform_int_distribution<int> distr(min, max);
    return distr(m_eng);
}
inline void updateComponents(PositionComponent& position, VelocityComponent& direction, DataComponent& data) {
    if ((data.thingy % 10) == 0) {
        if (position.x > position.y) {
            direction.x = static_cast<float>(random(-5, 5));
            direction.y = static_cast<float>(random(-10, 10));
        } else {
            direction.x = static_cast<float>(random(-10, 10));
            direction.y = static_cast<float>(random(-5, 5));
        }
    }
}

inline void updateData(DataComponent& data, TimeDelta dt) {
    data.thingy++;
    data.dingy += 0.0001 * static_cast<double>(dt);
    data.mingy = !data.mingy;
    std::string stringy = fmt::format(FMT_STRING("{:4.2f}"), data.dingy);
    std::char_traits<char>::copy(data.stringy, stringy.data(), std::min(stringy.length(), DataComponent::StringyMaxLength));
}


struct BenchmarkSettings {
    enum EMainType {
        Init = 1,
        Update = 2,
        Expand = 4,
        Churn = 8,
    } MainType;

};

static constexpr inline BenchmarkSettings::EMainType operator|(BenchmarkSettings::EMainType a, BenchmarkSettings::EMainType b) {
    return (BenchmarkSettings::EMainType)((int)a | (int)b);
}

template<BenchmarkSettings bs, BenchmarkSettings::EMainType type, typename FNRun>
static inline void bench_or_once(benchmark::State& state, FNRun&& run) {
    if constexpr (((int)bs.MainType & (int)type)) {
        for (auto _ : state)
            run();
    } else {
        run();
    }
}

constexpr BenchmarkSettings BsInit = { BenchmarkSettings::Init };
constexpr BenchmarkSettings BsUpdate = { BenchmarkSettings::Update };
constexpr BenchmarkSettings BsExpand = { BenchmarkSettings::Expand };
constexpr BenchmarkSettings BsChurn = { BenchmarkSettings::Churn };
