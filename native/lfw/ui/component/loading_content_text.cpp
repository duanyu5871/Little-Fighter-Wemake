#include "lfw/ui/component/loading_content_text.h"

#include <algorithm>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/lfw.h"
#include "lfw/ui/ui_parse.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

namespace {

bool reason_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// TS `v instanceof Error ? (v.message || v.name) : String(v)`：端口没有 Error 实体 ⇒ 一律
// `String(v)`（对象 = `[object Object]`，与 TS 的非 Error 对象一致；真 Error 记偏差）。
std::u16string reason_str(const Value& v) { return to_string(v); }

}

const ClazzTag* LoadingContentText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& LoadingContentText::TAGS() {
  static const std::vector<std::u16string> tags{u"LoadingContentText"};
  return tags;
}

double LoadingContentText::fade_out_duration() { return num(1).value_or(0.0); }

double LoadingContentText::fade_out_delay() { return num(2).value_or(0.0); }

void LoadingContentText::init() {
  if (fade_out_duration() == 0.0) return;
  const std::u16string expression =
      FadeOutOpacity::expression(fade_out_duration(), fade_out_delay()).done();
  const std::optional<CallFuncExpression> pr = parse_call_func_expression(expression);
  if (!pr.has_value()) return;

  Value desc(std::make_shared<Object>());
  Object* const o = as_object(desc);
  o->set(u"id", Value(pr->id));
  o->set(u"name", Value(pr->name));
  o->set(u"cls", Value(pr->name));
  Array args;
  for (const std::u16string& a : pr->args) args.push_back(Value(a));
  o->set(u"args", Value(std::make_shared<Array>(std::move(args))));
  o->set(u"enabled", Value(pr->enabled));

  Array components;
  components.push_back(desc);
  std::vector<std::unique_ptr<UIComponent>> created =
      lfw().factory.create_components(node, Value(std::make_shared<Array>(std::move(components))));
  if (created.empty() || created[0] == nullptr) return;
  _fadeout_owner = std::move(created[0]);
  fadeout = static_cast<FadeOutOpacity*>(_fadeout_owner.get());
  node.add_components(*fadeout);
}

void LoadingContentText::on_resume() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
  std::vector<std::function<void()>> removers;
  removers.push_back(lfw().callbacks.on(u"on_loading_start", [this](const LfwCallbacks::Payloads&) {
    on_loading_start();
  }));
  removers.push_back(lfw().callbacks.on(u"on_loading_end", [this](const LfwCallbacks::Payloads&) {
    on_loading_end();
  }));
  removers.push_back(lfw().callbacks.on(u"on_progress", [this](const LfwCallbacks::Payloads& p) {
    on_progress(p.empty() ? std::u16string() : p[0].text, p.empty() ? 0.0 : p[0].num);
  }));
  removers.push_back(
      lfw().callbacks.on(u"on_loading_failed", [this](const LfwCallbacks::Payloads& p) {
        on_loading_failed(p.empty() ? Value() : p[0].value);
      }));
  _remover = [removers]() {
    for (const std::function<void()>& r : removers) r();
  };
}

void LoadingContentText::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

void LoadingContentText::on_loading_start() { failed_reason.reset(); }

void LoadingContentText::on_loading_end() {
  failed_reason.reset();
  const std::optional<std::u16string> page = str(0);
  if (page.has_value() && !page->empty()) {
    UILayer* const l = node.layer();
    if (l != nullptr) {
      UIPushPageOpts opts;
      opts.id = *page;
      opts.has_id = true;
      l->set(opts);
    }
  } else {
    on_progress(u"waiting_others_players", 0.0);
  }
}

void LoadingContentText::on_progress(const std::u16string& text, double progress) {
  if (failed_reason.has_value()) return;
  if (fadeout != nullptr) fadeout->start(std::nullopt);
  const std::u16string str_text =
      progress != 0.0 ? u"loading: " + text + u"(" + number_to_string(progress) + u"%)"
                      : u" loading: " + text;
  node.set_text(str_text);
}

std::u16string LoadingContentText::format_reason(const Value& reason) {
  std::vector<std::u16string> texts;
  const auto push_one = [&texts](const Value& v, const Value& fallback_url) {
    if (reason_nullish(v)) return;
    const std::u16string msg = reason_str(v);
    Value url;
    if (const Object* const o = as_object(v)) {
      const Value* const cfg = o->get(u"config");
      const Object* const co = cfg != nullptr ? as_object(*cfg) : nullptr;
      const Value* const u = co != nullptr ? co->get(u"url") : nullptr;
      if (u != nullptr) url = *u;
    }
    if (reason_nullish(url)) url = fallback_url;
    std::u16string text = msg;
    const std::u16string* const us = std::get_if<std::u16string>(&url);
    if (us != nullptr && !us->empty() && msg.find(*us) == std::u16string::npos) {
      text = msg + u" (" + *us + u")";
    }
    if (!text.empty() && std::find(texts.begin(), texts.end(), text) == texts.end()) {
      texts.push_back(text);
    }
  };

  const Object* const ro = as_object(reason);
  const Value* const pairs = ro != nullptr ? ro->get(u"url_err_pair_list") : nullptr;
  const Array* const pa = pairs != nullptr ? as_array(*pairs) : nullptr;
  if (pa != nullptr && pa->size() > 0) {
    for (size_t i = 0; i < pa->size(); ++i) {
      const Array* const pair = as_array(pa->at(i));
      if (pair == nullptr || pair->size() < 2) continue;
      push_one(pair->at(1), pair->at(0));
    }
  } else {
    push_one(reason, Value());
  }
  if (texts.empty()) push_one(reason, Value());

  std::u16string out;
  for (size_t i = 0; i < texts.size(); ++i) {
    if (i != 0) out += u"; ";
    out += texts[i];
  }
  if (out.empty()) out = u"unknown error";
  return out;
}

void LoadingContentText::on_loading_failed(const Value& reason) {
  failed_reason = format_reason(reason);
  if (fadeout != nullptr) {
    fadeout->set_enabled(false);
    node.set_opacity(1.0);
  }
  node.set_text(u"loading failed: " + *failed_reason);
}

}
}
