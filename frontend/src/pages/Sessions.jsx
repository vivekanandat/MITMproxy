import { useEffect, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import api from "../lib/api";

export default function Sessions() {
  const [sessions, setSessions] = useState([]);
  const [name, setName] = useState("");
  const navigate = useNavigate();

  async function load() {
    const { data } = await api.get("/api/sessions");
    setSessions(data);
  }

  useEffect(() => {
    load();
  }, []);

  async function startCapture(e) {
    e.preventDefault();
    const { data } = await api.post("/api/sessions", { name });
    navigate(`/sessions/${data.id}/live`);
  }

  return (
    <div className="min-h-screen bg-gray-950 text-gray-100 p-8">
      <div className="max-w-3xl mx-auto space-y-6">
        <h1 className="text-2xl font-semibold">Capture Sessions</h1>

        <form onSubmit={startCapture} className="flex gap-2">
          <input
            placeholder="Session name (optional)"
            value={name}
            onChange={(e) => setName(e.target.value)}
            className="flex-1 px-3 py-2 rounded bg-gray-800 outline-none"
          />
          <button className="px-4 py-2 rounded bg-green-600 hover:bg-green-500 font-medium">
            Start New Capture
          </button>
        </form>

        <table className="w-full text-sm">
          <thead className="text-gray-400 text-left">
            <tr>
              <th className="py-2">Name</th>
              <th>Started</th>
              <th>Requests</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {sessions.map((s) => (
              <tr key={s.id} className="border-t border-gray-800">
                <td className="py-2">{s.name}</td>
                <td>{new Date(s.started_at).toLocaleString()}</td>
                <td>{s.request_count}</td>
                <td>
                  <Link className="text-blue-400 hover:underline" to={`/sessions/${s.id}`}>
                    View
                  </Link>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
