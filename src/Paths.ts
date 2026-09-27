import React from "react"
import type { RouteObject } from "react-router"

export enum Paths {
  _ = '',
  game = '/',
  editor = '/editor',
  bdy_editor = '/bdy_editor',
  itr_editor = '/itr_editor',
  frame_editor = '/frame_editor',
  workspaces_demo = '/workspaces',
  component_demos = '/component_demos',
  component_demos_InputNumber = '/component_demos/InputNumber',
  component_demos_index = '/component_demos/*',
  component_demos_Button = "/component_demos/Button",
  component_demos_Combine = "/component_demos/Combine",
  component_demos_Select = "/component_demos/Select",
  component_demos_Input = "/component_demos/Input",
  component_demos_Icon = "/component_demos/Icon",
  component_demos_Tag = "/component_demos/Tag",
  component_demos_Checkbox = "/component_demos/Checkbox",
  component_demos_Divider = "/component_demos/Divider",
  component_demos_Flex = "/component_demos/Flex",
  component_demos_Form = "/component_demos/Form",
  component_demos_TabButtons = "/component_demos/TabButtons",
  component_demos_Text = "/component_demos/Text",
  component_demos_TextArea = "/component_demos/TextArea",
  component_demos_Titled = "/component_demos/Titled",
  dat_viewer = "/dat_viewer",
  custom_game = "/custom_game",
  previewer = "/previewer",

  EntityInfoFormDemo = '/EntityInfoFormDemo',
  ArmorInfoFormDemo = '/ArmorInfoFormDemo',
  DrinkInfoFormDemo = '/DrinkInfoFormDemo',
  FrameInfoFormDemo = '/FrameInfoFormDemo',
  ChaseInfoFormDemo = '/ChaseInfoFormDemo',
  BdyInfoFormDemo = '/BdyInfoFormDemo',
  ItrInfoFormDemo = '/ItrInfoFormDemo',
  WpointInfoFormDemo = '/WpointInfoFormDemo',
  BpointInfoFormDemo = '/BpointInfoFormDemo',
  OpointInfoFormDemo = '/OpointInfoFormDemo',
  CpointInfoFormDemo = '/CpointInfoFormDemo',
}
export const Components: Record<Paths, React.ComponentType | null> = {
  [Paths._]: null,
  [Paths.game]: React.lazy(() => import("./App")),
  [Paths.editor]: React.lazy(() => import("./Editor")),
  [Paths.component_demos]: React.lazy(() => import("./pages/component_demos")),
  [Paths.component_demos_index]: () => null,
  [Paths.component_demos_InputNumber]: React.lazy(() => import("./pages/component_demos/InputNumberDemo")),
  [Paths.component_demos_Button]: React.lazy(() => import("./pages/component_demos/ButtonDemo")),
  [Paths.component_demos_Combine]: React.lazy(() => import("./pages/component_demos/CombineDemo")),
  [Paths.component_demos_Select]: React.lazy(() => import("./pages/component_demos/SelectDemo")),
  [Paths.component_demos_Input]: React.lazy(() => import("./pages/component_demos/InputDemo")),
  [Paths.component_demos_Icon]: React.lazy(() => import("./pages/component_demos/IconDemo")),
  [Paths.component_demos_Tag]: React.lazy(() => import("./pages/component_demos/TagDemo")),
  [Paths.component_demos_Checkbox]: React.lazy(() => import("./pages/component_demos/CheckboxDemo")),
  [Paths.component_demos_Divider]: React.lazy(() => import("./pages/component_demos/DividerDemo")),
  [Paths.component_demos_Flex]: React.lazy(() => import("./pages/component_demos/FlexDemo")),
  [Paths.component_demos_Form]: React.lazy(() => import("./pages/component_demos/FormDemo")),
  [Paths.component_demos_TabButtons]: React.lazy(() => import("./pages/component_demos/TabButtonsDemo")),
  [Paths.component_demos_Text]: React.lazy(() => import("./pages/component_demos/TextDemo")),
  [Paths.component_demos_TextArea]: React.lazy(() => import("./pages/component_demos/TextAreaDemo")),
  [Paths.component_demos_Titled]: React.lazy(() => import("./pages/component_demos/TitledDemo")),
  [Paths.workspaces_demo]: React.lazy(() => import("./pages/workspaces_demo")),
  [Paths.bdy_editor]: React.lazy(() => import("./EditorView/FrameEditorView/BdyEditor")),
  [Paths.itr_editor]: React.lazy(() => import("./EditorView/FrameEditorView/ItrEditor")),
  [Paths.frame_editor]: React.lazy(() => import("./EditorView/FrameEditorView")),
  [Paths.dat_viewer]: React.lazy(() => import("./pages/dat_viewer")),
  [Paths.custom_game]: React.lazy(() => import("./pages/custom_game")),
  [Paths.previewer]: React.lazy(() => import("./pages/previewer")),
  [Paths.EntityInfoFormDemo]: React.lazy(() => import("./EditorView/EntityInfoForm/demo")),
  [Paths.ArmorInfoFormDemo]: React.lazy(() => import("./EditorView/EntityInfoForm/ArmorInfoForm/demo")),
  [Paths.DrinkInfoFormDemo]: React.lazy(() => import("./EditorView/EntityInfoForm/DrinkInfoForm/demo")),
  [Paths.FrameInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/demo")),
  [Paths.ChaseInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/ChaseInfoForm/demo")),
  [Paths.BdyInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/BdyInfoForm/demo")),
  [Paths.ItrInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/ItrInfoForm/demo")),
  [Paths.WpointInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/WpointInfoForm/demo")),
  [Paths.BpointInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/BpointInfoForm/demo")),
  [Paths.OpointInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/OpointInfoForm/demo")),
  [Paths.CpointInfoFormDemo]: React.lazy(() => import("./EditorView/FrameInfoForm/CpointInfoForm/demo")),
}
export const Relations: { [x in Paths]?: Paths[] } = {
  [Paths._]: [
    Paths.game,
    Paths.component_demos,
    Paths.editor,
    Paths.workspaces_demo,
    Paths.bdy_editor,
    Paths.itr_editor,
    Paths.frame_editor,
    Paths.dat_viewer,
    Paths.custom_game,
    Paths.previewer,
    Paths.EntityInfoFormDemo,
    Paths.ArmorInfoFormDemo,
    Paths.DrinkInfoFormDemo,
    Paths.FrameInfoFormDemo,
    Paths.ChaseInfoFormDemo,
    Paths.BdyInfoFormDemo,
    Paths.ItrInfoFormDemo,
    Paths.WpointInfoFormDemo,
    Paths.BpointInfoFormDemo,
    Paths.OpointInfoFormDemo,
    Paths.CpointInfoFormDemo,
  ],
  [Paths.component_demos]: [
    Paths.component_demos_InputNumber,
    Paths.component_demos_Button,
    Paths.component_demos_Combine,
    Paths.component_demos_Select,
    Paths.component_demos_Input,
    Paths.component_demos_index,
    Paths.component_demos_Icon,
    Paths.component_demos_Tag,
    Paths.component_demos_Checkbox,
    Paths.component_demos_Divider,
    Paths.component_demos_Flex,
    Paths.component_demos_Form,
    Paths.component_demos_TabButtons,
    Paths.component_demos_Text,
    Paths.component_demos_TextArea,
    Paths.component_demos_Titled,
  ]
}

export const gen_route_obj = (path: Paths, parent?: Paths): RouteObject => {
  let str_path: string = path
  if (parent !== void 0) {
    if (path.startsWith(parent)) {
      str_path = path.replace(parent, '')
    }
    str_path = str_path.replace(/^\/(.*?)/, (_, a) => a)
  }
  const Component = Components[path] || (() => `component set as ${Components[path]}`);
  const ret: RouteObject = {
    path: str_path,
    element: React.createElement(React.Suspense, {}, React.createElement(Component))
  }
  if (Relations[path]) {
    ret.children = Relations[path].map((child_path) => gen_route_obj(child_path, path))
  }
  return ret;
}
export const Routes: RouteObject[] = Relations[Paths._]!.map(c => gen_route_obj(c))
