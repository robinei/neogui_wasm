
mergeInto(LibraryManager.library, {
    set_fill_color: function(r, g, b, a) {
        const style = "rgba(" + r + ", " + g + ", " + b + ", " + (a/255.0) + ")";
        canvasContext.fillStyle = style;
    },
    fill_rect: function(x, y, w, h) {
        canvasContext.fillRect(x, y, w, h);
    },
});
