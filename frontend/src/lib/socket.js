import { io } from "socket.io-client";

const SOCKET_BASE = import.meta.env.VITE_API_BASE || "http://localhost:4000";

let socket;

export function getSocket() {
  if (!socket) {
    socket = io(SOCKET_BASE, {
      autoConnect: false,
      auth: { token: localStorage.getItem("token") },
    });
  }
  return socket;
}
