(function () {
    var canvas = document.getElementById("skin-preview");
    var context = canvas.getContext("2d");
    var skinInput = document.getElementById("skin-file");
    var capeSelect = document.getElementById("cape-select");
    var modelSelect = document.getElementById("skin-model");
    var skin = null;
    var cape = null;
    var scale = 5;

    function parts(slim, legacy) {
        var arm = slim ? 3 : 4;
        var leftArmU = legacy ? 40 : 32;
        var leftArmV = legacy ? 16 : 48;
        var leftLegU = legacy ? 0 : 16;
        var leftLegV = legacy ? 16 : 48;
        return [
            { u: 8, v: 8, w: 8, h: 8, d: 8, x: 4, y: 0, overlay: [40, 8] },
            { u: 20, v: 20, w: 8, h: 12, d: 4, x: 4, y: 8, overlay: legacy ? null : [20, 36] },
            { u: 44, v: 20, w: arm, h: 12, d: 4, x: 4 - arm, y: 8, overlay: legacy ? null : [44, 36] },
            { u: leftArmU + 4, v: leftArmV + 4, w: arm, h: 12, d: 4, x: 12, y: 8, overlay: legacy ? null : [52, 52], mirror: legacy },
            { u: 4, v: 20, w: 4, h: 12, d: 4, x: 4, y: 20, overlay: legacy ? null : [4, 36] },
            { u: leftLegU + 4, v: leftLegV + 4, w: 4, h: 12, d: 4, x: 8, y: 20, overlay: legacy ? null : [4, 52], mirror: legacy }
        ];
    }

    function face(u, v, w, h, x, y, mirror) {
        if (mirror) {
            context.save();
            context.translate((x + w) * scale, y * scale);
            context.scale(-1, 1);
            context.drawImage(skin, u, v, w, h, 0, 0, w * scale, h * scale);
            context.restore();
            return;
        }

        context.drawImage(skin, u, v, w, h, x * scale, y * scale, w * scale, h * scale);
    }

    function drawView(offsetX, back) {
        var legacy = skin.height == 32;
        var list = parts(modelSelect.value == "slim", legacy);
        for (var i = 0; i < list.length; i++) {
            var part = list[i];
            var u = back ? part.u + part.w + part.d : part.u;
            var x = back ? 16 - part.x - part.w : part.x;
            face(u, part.v, part.w, part.h, offsetX + x, 2 + part.y, part.mirror);
            if (part.overlay) {
                var overlayU = back ? part.overlay[0] + part.w + part.d : part.overlay[0];
                face(overlayU, part.overlay[1], part.w, part.h, offsetX + x, 2 + part.y, part.mirror);
            }
        }

        if (back && cape) {
            context.drawImage(cape, 12, 1, 10, 16, (offsetX + 3) * scale, 10 * scale, 10 * scale, 16 * scale);
        }
    }

    function draw() {
        context.clearRect(0, 0, canvas.width, canvas.height);
        if (!skin) {
            return;
        }

        drawView(10, false);
        drawView(38, true);
    }

    function load(source, done) {
        var image = new Image();
        image.onload = function () {
            done(image);
            draw();
        };
        image.src = source;
    }

    function loadFile(input, done) {
        if (input.files.length > 0) {
            load(URL.createObjectURL(input.files[0]), done);
        }
    }

    canvas.width = 64 * scale;
    canvas.height = 36 * scale;
    context.imageSmoothingEnabled = false;
    load(canvas.dataset.skin || "/static/steve.png", function (image) { skin = image; });
    function selectCape() {
        var source = capeSelect.selectedOptions[0].dataset.url;
        if (!source) {
            cape = null;
            draw();
            return;
        }

        load(source, function (image) { cape = image; });
    }

    selectCape();

    skinInput.addEventListener("change", function () { loadFile(skinInput, function (image) { skin = image; }); });
    capeSelect.addEventListener("change", selectCape);
    modelSelect.addEventListener("change", draw);
})();
