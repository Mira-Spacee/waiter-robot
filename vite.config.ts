import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";
import path from "path";

// https://vitejs.dev/config/
export default defineConfig(() => ({
  server: {
    // Listen on all network interfaces (0.0.0.0)
    // This allows access from other devices on your local WiFi network
    host: "0.0.0.0",
    port: 8080,
    // Strict port - fail if port is already in use
    strictPort: true,
  },
  plugins: [react()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
}));
