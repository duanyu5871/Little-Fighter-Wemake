import React from "react";
import ReactDOM from "react-dom/client";
import { createHashRouter, RouterProvider } from "react-router-dom";
import { ConfigProvider } from "./Component/ConfigProvider";
import { install_desktop_update } from "./desktop_update";
import "./index.scss";
import './init';
import "./LFW/defines/defines";
import { Routes } from "./Paths";
import { start_version_check } from "./version_check";

const router = createHashRouter(Routes);
const root = document.getElementById("root")!;
root.addEventListener("dragover", (e) => e.preventDefault());
root.addEventListener("drop", (e) => e.preventDefault());

ReactDOM.createRoot(root).render(
  <React.StrictMode>
    <ConfigProvider >
      <RouterProvider router={router} />
    </ConfigProvider>
  </React.StrictMode>,
);

start_version_check();
install_desktop_update();