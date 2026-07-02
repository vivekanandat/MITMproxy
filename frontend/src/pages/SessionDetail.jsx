import { useEffect, useState } from "react";
import { useParams, Link } from "react-router-dom";
import { BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer } from "recharts";
import api from "../lib/api";

export default function SessionDetail() {
  const { id } = useParams();
  const [session, setSession] = useState(null);
  const [stats, setStats] = useState(null);
  const [requests, setRequests] = useState([]);

  useEffect(() => {
    api.get(`/api/sessions/${id}`).then((r) => setSession(r.data));
    api.get(`/api/sessions/${id}/stats`).then((r) => setStats(r.data));
    api.get(`/api/sessions/${id}/requests`).then((r) => setRequests(r.data));
  }, [id]);

  if (!session) return <div className="text-gray-400 p-8">Loading...</div>;

  return (
    <div className="min-h-screen bg-gray-950 text-gray-100 p-8">
      <div className="max-w-4xl mx-auto space-y-6">
        <Link to="/sessions" className="text-blue-400 text-sm hover:underline">
          ← Back to sessions
        </Link>
        <h1 className="text-2xl font-semibold">{session.name}</h1>
        <div className="flex gap-6 text-sm text-gray-400">
          <span>Total: {session.stats?.total_requests}</span>
          <span>Flagged: {session.stats?.flagged_count}</span>
          <span>Unique hosts: {session.stats?.unique_hosts}</span>
        </div>

        {stats && (
          <div className="bg-gray-900 rounded-lg p-4 h-56">
            <ResponsiveContainer width="100%" height="100%">
              <BarChart data={stats.topHosts}>
                <XAxis dataKey="host" stroke="#888" hide />
                <YAxis stroke="#888" allowDecimals={false} />
                <Tooltip />
                <Bar dataKey="count" fill="#3b82f6" />
              </BarChart>
            </ResponsiveContainer>
          </div>
        )}

        <table className="w-full text-sm">
          <thead className="text-gray-400 text-left">
            <tr>
              <th className="py-2">Time</th>
              <th>Method</th>
              <th>Host</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            {requests.map((r) => (
              <tr key={r.id} className="border-t border-gray-800">
                <td className="py-2">{new Date(r.timestamp).toLocaleTimeString()}</td>
                <td>{r.method}</td>
                <td>{r.host}</td>
                <td>{r.status_code}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
