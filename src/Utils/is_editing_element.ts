export function is_editing_element(target: EventTarget | null | undefined): boolean {
  if (!(target instanceof HTMLElement)) return false;
  if (target.isContentEditable) return true;
  switch (target.tagName) {
    case "TEXTAREA":
    case "SELECT":
      return true;
    case "INPUT": {
      const type = (target as HTMLInputElement).type?.toLowerCase();
      switch (type) {
        case "checkbox":
        case "radio":
        case "button":
        case "submit":
        case "reset":
        case "range":
        case "color":
        case "file":
        case "image":
          return false;
        default:
          return true;
      }
    }
    default:
      return false;
  }
}
