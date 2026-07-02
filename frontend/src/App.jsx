import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import Login from "./pages/Login";
import Sessions from "./pages/Sessions";
import LiveCapture from "./pages/LiveCapture";
import SessionDetail from "./pages/SessionDetail";
import ProtectedRoute from "./components/ProtectedRoute";
import Register from "./pages/Register";
import EndSession from "./pages/EndSession";

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/login" element={<Login />} />
        <Route path="/register" element={<Register />} />
        <Route
          path="/sessions"
          element={
            <ProtectedRoute>
              <Sessions />
            </ProtectedRoute>
          }
        />
        <Route
          path="/sessions/:id"
          element={
            <ProtectedRoute>
              <SessionDetail />
            </ProtectedRoute>
          }
        />
        <Route
          path="/sessions/:id/live"
          element={
            <ProtectedRoute>
              <LiveCapture />
            </ProtectedRoute>
          }
        />
        <Route
          path="/sessions/:id/end"
          element={
            <ProtectedRoute>
              <EndSession />
            </ProtectedRoute>
          }
        />
        <Route path="*" element={<Navigate to="/sessions" replace />} />
      </Routes>
    </BrowserRouter>
  );
}
