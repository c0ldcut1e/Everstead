const prefix = "ESBS:1.0/";
const impactValues = [["N", "None", 0, "No effect here."], ["L", "Low", 0.22, "Limited or temporary effect."], ["H", "High", 0.56, "Severe or lasting effect."]];
const groups = [
    {name: "Exploitability", description: "How easy the bug is to use."},
    {name: "Impact", description: "What the bug can do."},
    {name: "Modifiers", description: "How far and how long it spreads."},
];
const metrics = [
    {group: 0, key: "W", name: "Where", description: "Where the bug can be triggered.", values: [
        ["B", "Backend", 0.85, "Accounts, website, API or rendezvous, no game needed."],
        ["M", "Multiplayer", 0.75, "Any player joined to someone else's server."],
        ["H", "Host", 0.4, "Only the host or dedicated server owner."],
        ["S", "Singleplayer", 0.2, "Only in your own singleplayer world."],
    ]},
    {group: 0, key: "RQ", name: "Requirements", description: "What the player needs before it works.", values: [
        ["N", "None", 0.85, "Works right after joining."],
        ["C", "Common", 0.7, "Normal survival progress or common items."],
        ["R", "Rare", 0.5, "Rare items, a specific setup or another player's help."],
        ["O", "Operator", 0.25, "Operator or creative permissions."],
    ]},
    {group: 0, key: "RE", name: "Reliability", description: "How often it works when tried.", values: [
        ["A", "Always", 0.85, "Works every time."],
        ["S", "Sometimes", 0.6, "Needs timing or a few tries."],
        ["R", "Rarely", 0.35, "Hard to reproduce."],
    ]},
    {group: 0, key: "TL", name: "Tools", description: "What the attacker has to run.", values: [
        ["V", "Vanilla", 0.85, "Works with the normal game client."],
        ["M", "Modified", 0.62, "Needs a modified client or custom packets."],
    ]},
    {group: 0, key: "UI", name: "Victim action", description: "Whether another player has to do something.", values: [
        ["N", "None", 0.85, "Nobody else has to do anything."],
        ["R", "Required", 0.62, "A victim has to click, join, trade or accept something."],
    ]},
    {group: 1, key: "IT", name: "Items", description: "Duplicating, creating or deleting items and blocks.", values: impactValues},
    {group: 1, key: "WO", name: "World", description: "Griefing, bypassing protection or corrupting saves.", values: impactValues},
    {group: 1, key: "ST", name: "Stability", description: "Crashes, freezes or lag.", values: impactValues},
    {group: 1, key: "PL", name: "Players", description: "Kicking, killing or impersonating others, or seeing their data.", values: impactValues},
    {group: 1, key: "AD", name: "Advantage", description: "Unfair gameplay like flying, speed, reach or x-ray.", values: impactValues},
    {group: 1, key: "AC", name: "Accounts", description: "Account takeover, leaked emails, IPs or tokens.", values: impactValues},
    {group: 2, key: "RC", name: "Reach", description: "Who is affected.", values: [
        ["S", "Self", 0.5, "Only the player who triggers it."],
        ["O", "Server", 1, "Other players on the same server."],
        ["G", "Global", 1.15, "Every server or the backend."],
    ]},
    {group: 2, key: "PE", name: "Persistence", description: "How long the damage lasts.", values: [
        ["T", "Temporary", 0.9, "Goes away on its own or after relogging."],
        ["R", "Restart", 1, "Lasts until the server restarts."],
        ["P", "Permanent", 1.1, "Saved into the world or account."],
    ]},
    {group: 2, key: "DT", name: "Detection", description: "Whether admins would notice.", values: [
        ["O", "Obvious", 0.95, "Easy to see and trace back."],
        ["H", "Hidden", 1.05, "Hard to notice or leaves no trace."],
    ]},
];

const selected = {};
const vectorInput = document.getElementById("score-vector");

// We round on integers because float error can otherwise push a score up a whole step
function roundUp(value) {
    const scaled = Math.round(value * 100000);
    return scaled % 10000 === 0 ? scaled / 100000 : (Math.floor(scaled / 10000) + 1) / 10;
}

function weight(key) {
    return metrics.find(metric => metric.key === key).values.find(value => value[0] === selected[key])[2];
}

function score() {
    if (metrics.some(metric => !selected[metric.key])) {
        return null;
    }

    const impactKeys = ["IT", "WO", "ST", "PL", "AD", "AC"];
    const impact = 1 - impactKeys.reduce((product, key) => product * (1 - weight(key)), 1);
    if (impact <= 0) {
        return 0;
    }

    const exploitability = 11.38 * weight("W") * weight("RQ") * weight("RE") * weight("TL") * weight("UI");
    return roundUp(Math.min((6.42 * impact + exploitability) * weight("RC") * weight("PE") * weight("DT"), 10));
}

function severity(value) {
    if (value === null) {
        return "Incomplete";
    }

    if (value === 0) {
        return "None";
    }

    if (value < 4) {
        return "Low";
    }

    if (value < 7) {
        return "Medium";
    }

    return value < 9 ? "High" : "Critical";
}

function update(writeVector) {
    for (const button of document.querySelectorAll(".score-option")) {
        button.classList.toggle("active", selected[button.dataset.metric] === button.dataset.value);
    }

    const value = score();
    const label = severity(value);
    document.getElementById("score-number").textContent = value === null ? "-" : value.toFixed(1);
    const badge = document.getElementById("score-severity");
    badge.textContent = label;
    badge.dataset.severity = label.toLowerCase();
    const vector = prefix + metrics.filter(metric => selected[metric.key]).map(metric => metric.key + ":" + selected[metric.key]).join("/");
    if (writeVector) {
        vectorInput.value = vector;
    }

    history.replaceState(null, "", "#" + vector);
}

function parse(vector) {
    for (const part of vector.trim().replace(prefix, "").split("/")) {
        const [key, value] = part.split(":");
        const metric = metrics.find(metric => metric.key === key);
        if (metric && metric.values.some(entry => entry[0] === value)) {
            selected[key] = value;
        }
    }
}

const container = document.getElementById("score-metrics");
for (let i = 0; i < groups.length; i++) {
    const card = document.createElement("div");
    card.className = "score-group";
    const heading = document.createElement("h2");
    heading.textContent = groups[i].name;
    const subtitle = document.createElement("p");
    subtitle.className = "hint";
    subtitle.textContent = groups[i].description;
    card.append(heading, subtitle);
    for (const metric of metrics.filter(metric => metric.group === i)) {
        const row = document.createElement("div");
        row.className = "score-metric";
        const title = document.createElement("span");
        title.className = "score-metric-name";
        title.textContent = metric.name;
        title.title = metric.description;
        const options = document.createElement("div");
        options.className = "score-options";
        for (const [value, name, , tooltip] of metric.values) {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "score-option";
            button.dataset.metric = metric.key;
            button.dataset.value = value;
            button.title = tooltip;
            button.textContent = name;
            button.addEventListener("click", () => {
                selected[metric.key] = value;
                update(true);
            });
            options.appendChild(button);
        }

        row.append(title, options);
        card.appendChild(row);
    }

    container.appendChild(card);
}

vectorInput.addEventListener("input", () => {
    for (const key in selected) {
        delete selected[key];
    }

    parse(vectorInput.value);
    update(false);
});

parse(decodeURIComponent(location.hash.slice(1)));
update(true);
