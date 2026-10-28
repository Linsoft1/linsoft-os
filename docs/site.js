(function () {
  var lb = document.getElementById("lightbox"), im = lb.querySelector("img");
  document.querySelectorAll(".shots img").forEach(function (i) {
    i.addEventListener("click", function () { im.src = i.src; im.alt = i.alt; lb.hidden = false; });
  });
  lb.addEventListener("click", function () { lb.hidden = true; });
  document.addEventListener("keydown", function (e) { if (e.key === "Escape") lb.hidden = true; });
  document.querySelectorAll(".copy").forEach(function (b) {
    b.addEventListener("click", function () {
      var s = document.getElementById(b.dataset.copy).textContent;
      var p = navigator.clipboard ? navigator.clipboard.writeText(s) : Promise.reject();
      p.then(function () { b.textContent = "Skopírované ✓"; }, function () { b.textContent = "Označte a skopírujte"; });
    });
  });
})();
