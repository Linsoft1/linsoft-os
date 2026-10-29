(function () {
  var header = document.querySelector(".site-header");
  if (header) {
    function updateHeaderHeight() {
      document.documentElement.style.setProperty("--header-height", header.getBoundingClientRect().height + "px");
    }
    updateHeaderHeight();
    if (typeof ResizeObserver !== "undefined") {
      new ResizeObserver(updateHeaderHeight).observe(header);
    } else {
      window.addEventListener("resize", updateHeaderHeight);
    }
  }
  var toggle = document.querySelector(".menu-toggle");
  if (toggle) {
    var nav = toggle.closest("nav");
    toggle.hidden = false;
    nav.classList.add("nav-ready");
    function closeMenu() { toggle.setAttribute("aria-expanded", "false"); }
    toggle.addEventListener("click", function () {
      toggle.setAttribute("aria-expanded", String(toggle.getAttribute("aria-expanded") !== "true"));
    });
    nav.querySelectorAll(".nav-links a").forEach(function (link) {
      link.addEventListener("click", closeMenu);
    });
    nav.addEventListener("keydown", function (event) {
      if (event.key === "Escape" && toggle.getAttribute("aria-expanded") === "true") {
        closeMenu();
        toggle.focus();
      }
    });
  }
  var lb = document.getElementById("lightbox");
  if (lb) {
    var im = lb.querySelector("img");
    document.querySelectorAll(".shots img").forEach(function (i) {
      i.addEventListener("click", function () { im.src = i.src; im.alt = i.alt; lb.hidden = false; });
    });
    lb.addEventListener("click", function () { lb.hidden = true; });
    document.addEventListener("keydown", function (e) { if (e.key === "Escape") lb.hidden = true; });
  }
  document.querySelectorAll(".copy").forEach(function (b) {
    b.addEventListener("click", function () {
      var s = document.getElementById(b.dataset.copy).textContent;
      var p = navigator.clipboard ? navigator.clipboard.writeText(s) : Promise.reject();
      p.then(function () { b.textContent = "Skopírované ✓"; }, function () { b.textContent = "Označte a skopírujte"; });
    });
  });
})();
