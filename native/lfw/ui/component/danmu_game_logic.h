#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"
#include "lfw/helper/join_queue.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/camera_ctrl.h"
#include "lfw/ui/component/summary_logic.h"
#include "lfw/utils/times.h"

namespace lfw {

class Entity;

namespace ui {

// TS `ui/component/DanmuGameLogic.ts`（`TAGS = ["DanmuGameLogic"]`）：弹幕互动三种模式
// （ffa / teams8 / coop）的选人、应援、换人、队伍平衡与镜头跟随。
class DanmuGameLogic : public SummaryLogic {
 public:
  static constexpr const char* TAG = "DanmuGameLogic";
  static constexpr const char16_t* BROADCAST_ON_START = u"DanmuGameLogic_ON_START";
  static constexpr const char16_t* BROADCAST_ON_STOP = u"DanmuGameLogic_ON_STOP";
  static constexpr double MAX_FIGHTERS = 32;
  static constexpr double COOP_TEMPLATES = 4;
  static constexpr double TEAMS8_TEAM_CAP = 4;
  static constexpr double STAGE_SWITCH_DELAY = 60 * 3;
  static constexpr double FFA_SEEDS = 8;
  static constexpr double CHEER_CD = 60 * 3;
  static constexpr double CHEER_HP_RATIO = 0.15;
  static constexpr double QUEUE_IDLE_TIMEOUT = 60 * 300;

  struct ViewerStat {
    std::u16string uid;
    std::u16string name;
    double spawns = 0;
    double kills = 0;
    double deads = 0;
    double damages = 0;
    double cheers = 0;
    bool alive = false;
  };

  struct CheerEvent {
    std::u16string uid;
    std::u16string name;
    double time = 0;
  };

  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  DanmuGameLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  const std::u16string& mode();
  const std::vector<CheerEvent>& cheer_feed() const { return _cheer_feed; }
  std::u16string stage_name();
  bool waiting_next_stage() const { return _waiting_next_stage; }

  double time = 0;

  void update_teams();
  void on_fighter_add(Entity& e) override;
  void on_fighter_del(Entity& e) override;
  void on_start() override;
  void on_stop() override;
  void update(double dt) override;

  void update_bg();
  bool join(const helper::Entrant& entrant);
  bool leave(const std::u16string& uid);
  void touch(const std::u16string& uid);
  bool enter(const helper::Entrant& entrant);
  bool switch_to(const std::u16string& uid, const Value& oid = Value());
  bool cheer(const std::u16string& uid);
  std::vector<ViewerStat> viewer_stats();
  void update_staring();

  helper::JoinQueue join_queue{50};

  // 台面观察口（不参与逻辑）。
  const std::vector<std::pair<std::u16string, double>>& team_caps() const { return _team_caps; }
  const std::map<std::u16string, ViewerStat>& viewers() const { return _viewers; }
  CameraCtrl* cam_ctrl() const { return _cam_ctrl; }
  bool coop_stage_ready() const { return _coop_stage_ready; }
  bool waiting_respawn() const { return _waiting_respawn; }
  const std::set<std::u16string>& teams_set() const { return _teams; }
  double staring_countdown() const { return _staring_countdown.value(); }
  double gameover_countdown() const { return _gameover_countdown.value(); }
  double next_stage_delay() const { return _next_stage_delay.value(); }
  double queue_sweep_timer() const { return _queue_sweep_timer.value(); }
  const std::map<std::u16string, double>& cheer_cd() const { return _cheer_cd; }
  const std::map<std::u16string, double>& queue_seen() const { return _queue_seen; }
  const std::optional<std::vector<helper::Entrant>>& survivors() const { return _survivors; }
  const std::optional<std::u16string>& join_fallen() const { return _join_fallen; }

 protected:
  void sweep_queue();
  void try_join();
  bool claim_queued();
  const std::vector<Value>& regular_fighter_datas();
  std::optional<Value> template_data();
  Entity* find_claimable_template();
  bool claim_template(const helper::Entrant& entrant);
  void place_coop_spawn(Entity& e);
  void fill_templates();
  std::optional<Value> regular_data(const Value& oid);
  Entity* spawn_entrant(const helper::Entrant& entrant, const std::u16string& team,
                        const Value& data = Value());
  void track_viewer(Entity& e);
  void untrack_viewer(Entity& e);
  Entity* find_viewer_fighter(const std::u16string& uid);
  bool start_coop_stage();
  std::vector<helper::Entrant> collect_survivors();
  void respawn_viewers();
  std::vector<Entity*> staring_candidates();
  std::optional<std::u16string> find_join_team();
  void fighter_enter(Entity& v);

  void handle_world_stage_change(stage::Stage* stage, stage::Stage* prev);
  void handle_stage_finish();

 private:
  using ValueListener = NoEmitCallbacksT<Value>::Listener;
  using WorldListener = NoEmitCallbacksT<WorldCallbackArgs>::Listener;
  using StageListener = stage::StageCallbacks::Listener;

  struct WorldListenerT : WorldListener {
    explicit WorldListenerT(DanmuGameLogic* o);
  };
  struct StageListenerT : StageListener {
    explicit StageListenerT(DanmuGameLogic* o);
  };
  struct SummaryListenerT : ValueListener {
    explicit SummaryListenerT(ViewerStat* s);
  };
  struct EntityListenerT : ValueListener {
    explicit EntityListenerT(ViewerStat* s);
  };
  struct ViewerTrack {
    std::unique_ptr<SummaryListenerT> sum;
    std::unique_ptr<EntityListenerT> ent;
  };

  Times _staring_countdown{0, 60 * 30};
  Times _gameover_countdown{0, 60 * 5};
  std::set<std::u16string> _teams;
  std::vector<std::pair<std::u16string, double>> _team_caps;
  std::optional<std::u16string> _join_fallen;
  std::optional<std::vector<helper::Entrant>> _survivors;
  bool _waiting_respawn = false;
  bool _waiting_next_stage = false;
  bool _coop_stage_ready = false;
  Times _next_stage_delay{0, STAGE_SWITCH_DELAY};
  std::map<std::u16string, double> _queue_seen;
  Times _queue_sweep_timer{0, 60};
  std::optional<std::vector<Value>> _regular_datas;
  CameraCtrl* _cam_ctrl = nullptr;
  std::map<std::u16string, ViewerStat> _viewers;
  std::map<std::u16string, ViewerTrack> _viewer_tracks;
  std::map<std::u16string, double> _cheer_cd;
  std::vector<CheerEvent> _cheer_feed;

  WorldListenerT _world_cb{this};
  StageListenerT _stage_cb{this};

  friend struct WorldListenerT;
  friend struct StageListenerT;
  friend struct SummaryListenerT;
  friend struct EntityListenerT;
};

}
}
