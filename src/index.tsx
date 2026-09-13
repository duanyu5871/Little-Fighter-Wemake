import React from "react";
import ReactDOM from "react-dom/client";
import { createHashRouter, RouterProvider } from "react-router-dom";
import "./index.scss";
import './init';
import "./LFW/defines/defines";
import { Paths } from "./Paths";
import { ConfigProvider } from "./Component/ConfigProvider";
import { start_version_check } from "./version_check";

const router = createHashRouter(Paths.Routes);
ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <ConfigProvider >
      <RouterProvider router={router} />
    </ConfigProvider>
  </React.StrictMode>,
);

start_version_check();