// Resolve the controls once after the deferred script loads.
const byId = id => document.getElementById(id);
const angle = byId("angle");

// Requests always reach the ESP so repeated clicks generate repeated commands.
async function send(route) {
  const response = await fetch(route, {cache: "no-store"});
  if (!response.ok) throw new Error("ESP rejected the request"); // ERROR handling
}

// Only report delivery to the ESP. The Mega selects the active driving mode.
async function act(route) {
  try {
    await send(route);
    byId("status").textContent = "Sent to ESP";
  } catch {
    byId("status").textContent = "Command failed. Check the ESP connection."; // ERROR handling
  }
}

// Distance and cardinal buttons each carry their endpoint in the HTML.
for (const button of document.querySelectorAll("[data-route]")) {
  button.onclick = () => act(button.dataset.route);
}

// Show the angle while dragging and submit on release or deliberate repeat.
angle.oninput = () => byId("angleValue").textContent = angle.value + "°";
angle.onchange = () => act("/compass?value=" + angle.value);
byId("repeat").onclick = angle.onchange;

// Validate before transmission and show the warning beside the speed box.
byId("speedForm").onsubmit = event => {
  event.preventDefault();
  const value = byId("speed").value.trim();
  if (!/^\d+$/.test(value) || Number(value) > 255) { // ERROR handling
    byId("error").textContent = "Enter a whole number from 0 to 255."; // ERROR handling
    return; // ERROR handling
  }
  byId("error").textContent = "";
  act("/speed?value=" + Number(value));
};

// ERROR handling: only a visible controller sends heartbeats. Avoid overlapping pings.
let pending = false;
async function heartbeat() {
  if (document.hidden || pending) return; // ERROR handling
  pending = true;
  try { await send("/ping"); } catch { /* ERROR handling: Mega stops on expiry. */ }
  finally { pending = false; }
}
heartbeat();
setInterval(heartbeat, 500); // ERROR handling
document.addEventListener("visibilitychange", heartbeat);
