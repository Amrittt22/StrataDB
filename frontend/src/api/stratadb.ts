
const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL || "http://localhost:8080";

export interface KVEntry {
  key: string;
  value: string;
}

async function request<T>(
  path: string,
  options: RequestInit = {}
): Promise<T> {
  const response = await fetch(`${API_BASE_URL}${path}`, options);

  const text = await response.text();

  let data: any = {};

  try {
    data = text ? JSON.parse(text) : {};
  } catch {
    data = { value: text };
  }

  if (!response.ok || data.success === false) {
    throw new Error(
      data.error || data.message || `Request failed: ${response.status}`
    );
  }

  return data as T;
}

export const strataDB = {
  health: () =>
    request<any>("/health"),

  put: (key: string, value: string) =>
    request<any>(`/kv/${encodeURIComponent(key)}`, {
      method: "PUT",
      headers: { "Content-Type": "text/plain" },
      body: value,
    }),

  get: (key: string) =>
    request<any>(`/kv/${encodeURIComponent(key)}`),

  delete: (key: string) =>
    request<any>(`/kv/${encodeURIComponent(key)}`, {
      method: "DELETE",
    }),

  scan: (start: string, end: string) =>
    request<any>(
      `/scan?start=${encodeURIComponent(start)}&end=${encodeURIComponent(end)}`
    ),

  stats: () =>
    request<any>("/stats"),
};
