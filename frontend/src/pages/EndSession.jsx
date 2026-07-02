import { useEffect, useState } from "react";
import { useParams, useNavigate } from "react-router-dom";
import api from "../lib/api";

export default function EndSession() {
  const { id } = useParams();
  const navigate = useNavigate();
  const [status, setStatus] = useState("Ending session...");

  useEffect(() => {
    api
      .patch(`/api/sessions/${id}/end`)
      .then(() => {
        setStatus("Session ended.");
        setTimeout(() => navigate("/sessions"), 1000);
      })
      .catch((err) => {
        setStatus(err.response?.data?.error || "Failed to end session");
      });
  }, [id]);

  return (
    <div className="min-h-screen flex items-center justify-center bg-gray-950 text-gray-100">
      {status}
    </div>
  );
}
